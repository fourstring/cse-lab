#ifndef ydb_server_2pl_h
#define ydb_server_2pl_h

#include <string>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <condition_variable>
#include <memory>
#include "extent_client.h"
#include "lock_protocol.h"
#include "lock_client.h"
#include "lock_client_cache.h"
#include "ydb_protocol.h"
#include "ydb_server.h"

using operation_set_t = std::unordered_map<lock_protocol::lockid_t, std::string>;
enum class AccessKind {
    Read,
    Write
};


struct TwoPLTransaction {
    operation_set_t read_set;
    operation_set_t write_set;
    std::unordered_map<lock_protocol::lockid_t, AccessKind> lock_set;
};

using twopl_trans_set_t = std::unordered_map<ydb_protocol::transaction_id, TwoPLTransaction>;
using rag_t = std::unordered_map<int, std::unordered_set<int>>; // Resources Allocation Graph
using readers_t = std::unordered_set<ydb_protocol::transaction_id>;

struct TransAccess {
    AccessKind kind = AccessKind::Read;
    readers_t readers;
    ydb_protocol::transaction_id writer = 0;
    int upgrader = 0;

    TransAccess() = default;

    TransAccess(AccessKind kind, readers_t readers, int writer);

    inline bool is_free() {
        return (kind == AccessKind::Read && readers.empty()) || (kind == AccessKind::Write && writer == 0);
    }

    bool has_access(ydb_protocol::transaction_id tid, AccessKind kind);

    bool add_reader(ydb_protocol::transaction_id tid);

    void remove_reader(ydb_protocol::transaction_id tid);

    bool set_writer(ydb_protocol::transaction_id tid);

    void clear_writer(ydb_protocol::transaction_id tid);

    bool try_upgrade(ydb_protocol::transaction_id tid);

    bool can_upgrade_now();

    inline bool is_upgradable(ydb_protocol::transaction_id tid);

    inline bool is_sharable(ydb_protocol::transaction_id tid);
};

using access_trace_t = std::unordered_map<lock_protocol::lockid_t, TransAccess>;
using cv_ptr = std::unique_ptr<std::condition_variable>;

struct Waiter {
    cv_ptr wait_read{new std::condition_variable{}};
    cv_ptr wait_write{new std::condition_variable{}};
    cv_ptr wait_upgrade{new std::condition_variable{}};
    std::unordered_set<ydb_protocol::transaction_id> write_waiters{};
};

using condition_t = std::unordered_map<lock_protocol::lockid_t,
        Waiter>;

class ydb_server_2pl : public ydb_server {
private:
    std::mutex internal_state_mutex;
    twopl_trans_set_t transactions;
    rag_t rag;
    access_trace_t access_trace;
    condition_t waiters;
public:
    ydb_server_2pl(std::string, std::string);

    ~ydb_server_2pl();

    ydb_protocol::status transaction_begin(int, ydb_protocol::transaction_id &);

    ydb_protocol::status transaction_commit(ydb_protocol::transaction_id, int &);

    ydb_protocol::status transaction_abort(ydb_protocol::transaction_id, int &);

    ydb_protocol::status get(ydb_protocol::transaction_id id, std::string &key, std::string &ret);

    ydb_protocol::status set(ydb_protocol::transaction_id tid, std::string &key, std::string &value, int &);

    ydb_protocol::status del(ydb_protocol::transaction_id tid, std::string &key, int &);

    void clean_transaction(ydb_protocol::transaction_id tid);

    ydb_protocol::status
    acquire_or_upgrade_lock(ydb_protocol::transaction_id tid, lock_protocol::lockid_t lid, AccessKind kind);

    ydb_protocol::status release_lock(ydb_protocol::transaction_id tid, lock_protocol::lockid_t lid, AccessKind kind);

    bool deadlock_detect(int rid);
};

#endif

