#include "ydb_server_2pl.h"
#include "extent_client.h"
#include <iterator>
#include <algorithm>
#include <utility>
#include "tprintf.h"
#include <stack>

//#define DEBUG 1

ydb_server_2pl::ydb_server_2pl(std::string extent_dst, std::string lock_dst) : ydb_server(extent_dst, lock_dst) {

}

ydb_server_2pl::~ydb_server_2pl() {
}

ydb_protocol::status ydb_server_2pl::transaction_begin(int,
                                                       ydb_protocol::transaction_id &out_id) {    // the first arg is not used, it is just a hack to the rpc lib
    // lab3: your code here
    int _ = 0;
    ydb_protocol::transaction_id tid;
    ydb_server::transaction_begin(_, tid);
    tprintf("Transaction %d begin\n", tid)
    unique_t u{internal_state_mutex};
    {
        auto[e, ok] = transactions.try_emplace(tid);
        if (!ok) {
            return ydb_protocol::TRANSIDINV;
        }
    }
    {
        auto[e, ok] = rag.try_emplace(tid);
        if (!ok) {
            transactions.erase(tid);
            return ydb_protocol::TRANSIDINV;
        }
    }
    out_id = tid;
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_2pl::transaction_commit(ydb_protocol::transaction_id id, int &) {
    // lab3: your code here
    auto &trans = transactions[id];
    for (auto &[key, value]:trans.write_set) {
        ec->put(key, value);
    }
    clean_transaction(id);
    tprintf("Transaction %d committed\n", id)
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_2pl::transaction_abort(ydb_protocol::transaction_id id, int &) {
    // lab3: your code here
    clean_transaction(id);
    tprintf("Transaction %d aborted\n", id)
    return ydb_protocol::OK;
}

ydb_protocol::status
ydb_server_2pl::get(ydb_protocol::transaction_id id, std::string &key, std::string &ret) {
    // lab3: your code here
    auto &trans = transactions[id];
    auto lid = hasher(key) % 1024;
    //tprintf("Transaction %d start to get %s, hashed as %lu\n", id, key.data(), lid)
    if (acquire_or_upgrade_lock(id, lid, AccessKind::Read) == ydb_protocol::ABORT) {
        int _;
        transaction_abort(id, _);
        return ydb_protocol::ABORT;
    }
    try {
        ret = trans.write_set.at(lid);
        return ydb_protocol::OK;
    } catch (std::out_of_range &e) {

    }
    try {
        ret = trans.read_set.at(lid);
        return ydb_protocol::OK;
    } catch (std::out_of_range &e) {

    }
    ec->get(lid, ret);
    trans.read_set[lid] = ret;
    //tprintf("Transaction %d has got %s, hashed as %lu, successfully: %s\n", id, key.data(), lid, ret.data())
    return ydb_protocol::OK;
}

ydb_protocol::status
ydb_server_2pl::set(ydb_protocol::transaction_id id, std::string &key, std::string &value, int &) {
    // lab3: your code here
    auto &trans = transactions[id];
    auto lid = hasher(key) % 1024;
    //tprintf("Transaction %d start to set %s, hashed as %lu\n", id, key.data(), lid)
    if (acquire_or_upgrade_lock(id, lid, AccessKind::Write) == ydb_protocol::ABORT) {
        int _;
        transaction_abort(id, _);
        return ydb_protocol::ABORT;
    }
    trans.write_set[lid] = value;
    //tprintf("Transaction %d has set %s, hashed as %lu, successfully: %s\n", id, key.data(), lid, value.data())
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_2pl::del(ydb_protocol::transaction_id id, std::string &key, int &) {
    // lab3: your code here
    int _;
    return set(id, key, empty, _);
}

void ydb_server_2pl::clean_transaction(ydb_protocol::transaction_id tid) {
    try {
        auto &t = transactions.at(tid);
        for (auto[lid, kind]: t.lock_set) {
            release_lock(tid, lid, kind);
        }
        unique_t u{internal_state_mutex};
        for (auto[lid, kind]: t.lock_set) {
            auto &waiter = waiters[lid];
            waiter.write_waiters.erase(tid);
        }
        transactions.erase(tid);
        rag.erase(tid);
    } catch (std::out_of_range &e) {

    }
}

ydb_protocol::status
ydb_server_2pl::acquire_or_upgrade_lock(ydb_protocol::transaction_id tid, lock_protocol::lockid_t lid,
                                        AccessKind kind) {
    unique_t u{internal_state_mutex};
    auto iaccess = access_trace.find(lid);
    auto iwaiter = waiters.find(lid);
    if (iwaiter == waiters.end()) {
        waiters.try_emplace(lid);
    }

    auto rag_finalizer = [&]() {
        rag[tid].erase(-lid);
        rag[-lid].insert(tid);
        transactions[tid].lock_set[lid] = kind;
    };
    // Add request in rag
    if (rag.find(-lid) == rag.end()) {
        rag.try_emplace(-lid);
    }
    rag[tid].insert(-lid);

    tprintf("Transaction %d requested %s lock %llu\n", tid, kind == AccessKind::Read ? "Read" : "Write", lid)

    if (iaccess == access_trace.end()) {
        if (kind == AccessKind::Read) {
            access_trace.try_emplace(lid, kind, readers_t{tid}, 0);
        } else {
            access_trace.try_emplace(lid, kind, readers_t{}, tid);
        }
        tprintf("Granted %s Lock %llu to trans %d\n", kind == AccessKind::Read ? "Read" : "Write", lid, tid)
        rag_finalizer();
        return ydb_protocol::OK;
    }
    auto &access = access_trace[lid];
    if (access.has_access(tid, kind)) {
        rag_finalizer();
        return ydb_protocol::OK;
    }

    if (access.is_free()) {
        if (kind == AccessKind::Read) {
            access.add_reader(tid);
        } else {
            access.set_writer(tid);
        }
        rag_finalizer();
        tprintf("Granted %s Lock %llu to trans %d\n", kind == AccessKind::Read ? "Read" : "Write", lid, tid)
        return ydb_protocol::OK;
    }
    auto fail_to_upgrade = false;
    if (kind == AccessKind::Read && access.is_sharable(tid)) {
        access.readers.insert(tid);
        rag_finalizer();
        tprintf("Shared %s Lock %llu to trans %d\n", kind == AccessKind::Read ? "Read" : "Write", lid, tid)
        return ydb_protocol::OK;
    }
    if (kind == AccessKind::Write && access.is_upgradable(tid)) {
        if (access.try_upgrade(tid)) {
            rag_finalizer();
            tprintf("Upgraded %s Lock %llu to trans %d\n", kind == AccessKind::Read ? "Read" : "Write", lid, tid)
            return ydb_protocol::OK;
        } else {
            fail_to_upgrade = true;
        }
    }
    // Necessary to avoid meaningless wait
    if (!fail_to_upgrade && deadlock_detect(tid)) {
        rag[tid].erase(-lid);
        tprintf("Deadlock detected: trans %d try to get %s lock %llu\n", tid,
                kind == AccessKind::Read ? "Read" : "Write",
                lid)
        return ydb_protocol::ABORT;
    }
    // start waiting
    auto try_lock = [&]() -> bool {
        // rag may change during waiting, must recheck
        if (deadlock_detect(tid)) {
            rag[tid].erase(-lid);
            tprintf("Deadlock detected: trans %d try to get %s lock %llu\n", tid,
                    kind == AccessKind::Read ? "Read" : "Write",
                    lid)
            throw ydb_protocol::ABORT;
        }
        if (fail_to_upgrade) {
            return access.try_upgrade(tid);
        }
        if (kind == AccessKind::Read) {
            return access.add_reader(tid);
        } else {
            return access.set_writer(tid);
        }
    };

    do {
        auto &waiter = waiters[lid];
        tprintf("Transaction %d started to wait for %s lock %llu\n", tid, kind == AccessKind::Read ? "Read" : "Write",
                lid)
        if (fail_to_upgrade) {
            waiter.wait_upgrade->wait(u);
        } else {
            if (kind == AccessKind::Read) {
                waiter.wait_read->wait(u);
            } else {
                waiter.write_waiters.insert(tid);
                tprintf("%llu has %lu waiters currently\n", lid, waiter.write_waiters.size())
                waiter.wait_write->wait(u);
            }
        }
        tprintf("Transaction %d awoke, for %s lock %llu\n", tid, kind == AccessKind::Read ? "Read" : "Write",
                lid)
        try {
            if (try_lock()) {
                if (kind == AccessKind::Write && !fail_to_upgrade) {
                    waiter.write_waiters.erase(tid);
                }
                break;
            }
        } catch (ydb_protocol::status s) {
            return s;
        }
    } while (true);
    rag_finalizer();
    tprintf("Granted %s Lock %llu to trans %d\n", kind == AccessKind::Read ? "Read" : "Write", lid, tid)
    return ydb_protocol::OK;
}

ydb_protocol::status
ydb_server_2pl::release_lock(ydb_protocol::transaction_id tid, lock_protocol::lockid_t lid, AccessKind kind) {
    unique_t u{internal_state_mutex};
    auto &access = access_trace[lid];
    tprintf("Released %s Lock %llu from trans %d\n", kind == AccessKind::Read ? "Read" : "Write", lid, tid)
    if (kind == AccessKind::Read) {
        access.remove_reader(tid);
    } else {
        access.clear_writer(tid);
    }
    rag[-lid].erase(tid);
    auto &waiter = waiters[lid];
    if (access.can_upgrade_now()) {
        tprintf("Lock %llu will upgrade now\n", lid)
        waiter.wait_upgrade->notify_one();
        return ydb_protocol::OK;
    }
    if (access.is_free()) {
        tprintf("%s Lock %llu is free now\n", kind == AccessKind::Read ? "Read" : "Write", lid)
        if (!waiter.write_waiters.empty()) {
            waiter.wait_write->notify_all();
        } else {
            waiter.wait_read->notify_all();
        }
    }
    return ydb_protocol::OK;
}

bool ydb_server_2pl::deadlock_detect(ydb_protocol::transaction_id rid) {
    std::unordered_set<int> visited;
    std::unordered_map<int, int> pre, low;
    std::stack<int> scc_stack;

    auto clear = [&]() {
        visited.clear();
    };

    auto tarjan_search = [&, this](auto &self, int parent, int clock) -> int {
        pre[parent] = clock;
        low[parent] = clock;
        auto min_child_low = 0x7FFFFFFF;
        auto min_backtrack_pre = 0x7FFFFFFF;
        scc_stack.push(parent);
        for (auto &child:this->rag[parent]) {
            if (visited.find(child) == visited.end()) {
                visited.insert(child);
                auto child_low = self(self, child, clock + 1);
                if (child_low < min_child_low) {
                    min_child_low = child_low;
                }
            } else {
                if (child != parent && pre[child] < min_backtrack_pre) {
                    min_backtrack_pre = pre[child];
                }
            }
        }
        low[parent] = std::min({low[parent], min_child_low, min_backtrack_pre});
        if (low[parent] == pre[parent]) {
            std::vector<int> scc{};
            while (scc_stack.top() != parent) {
                scc.push_back(scc_stack.top());
                scc_stack.pop();
            }
            scc.push_back(scc_stack.top());
            scc_stack.pop();
            if (scc.size() > 2) {
                if (std::find(scc.begin(), scc.end(), rid) != scc.end()) {
                    throw rid;
                }
            }
        }
        return low[parent];
    };

    for (auto &res:rag) {
        clear();
        try {
            tarjan_search(tarjan_search, res.first, 1);
        } catch (ydb_protocol::transaction_id r) {
            return true;
        }
    }

    return false;
}


bool TransAccess::set_writer(ydb_protocol::transaction_id tid) {
    if (kind == AccessKind::Read && !readers.empty()) {
        return false;
    } else if (kind == AccessKind::Write && writer != 0) {
        return false;
    }
    kind = AccessKind::Write;
    writer = tid;
    upgrader = 0;
    return true;
}

bool TransAccess::add_reader(ydb_protocol::transaction_id tid) {
    if (kind == AccessKind::Write && writer != 0) {
        return false;
    }
    kind = AccessKind::Read;
    readers.insert(tid);
    return true;
}

void TransAccess::clear_writer(ydb_protocol::transaction_id tid) {
    if (writer == tid) {
        writer = 0;
    }
}

void TransAccess::remove_reader(ydb_protocol::transaction_id tid) {
    readers.erase(tid);
}

bool TransAccess::try_upgrade(ydb_protocol::transaction_id tid) {
    if (readers.size() == 1) {
        kind = AccessKind::Write;
        readers.clear();
        writer = tid;
        upgrader = 0;
        return true;
    } else {
        upgrader = tid;
        return false;
    }
}

bool TransAccess::has_access(ydb_protocol::transaction_id tid, AccessKind kind) {
    if (kind == this->kind) {
        if (kind == AccessKind::Read && readers.find(tid) != readers.end()) {
            return true;
        } else if (kind == AccessKind::Write && writer == tid) {
            return true;
        }
    }
    return false;
}

TransAccess::TransAccess(AccessKind kind, readers_t readers, int writer) : kind(kind), readers(std::move(readers)),
                                                                           writer(writer) {}

inline bool TransAccess::is_upgradable(ydb_protocol::transaction_id tid) {
    return upgrader == 0 && kind == AccessKind::Read && readers.find(tid) != readers.end();
}

bool TransAccess::is_sharable(ydb_protocol::transaction_id tid) {
    return kind == AccessKind::Read && upgrader == 0;
}

bool TransAccess::can_upgrade_now() {
    return kind == AccessKind::Read && readers.find(upgrader) != readers.end() && readers.size() == 1;
}



