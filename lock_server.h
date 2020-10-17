// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <unordered_set>
#include <unordered_map>
#include <condition_variable>
#include <memory>
#include <mutex>

struct LockState {
    LockKind kind{LockKind::Shared};
    LockStatus status{LockStatus::Unlocked};
    std::unordered_set<int> owned_clients{};

    LockState() = default;

    explicit LockState(LockKind kind) : kind(kind) {

    }

    LockState(LockState &&other) noexcept: kind{other.kind}, status{other.status},
                                           owned_clients{std::move(other.owned_clients)} {

    }

    LockState &operator=(LockState &&rhs) noexcept {
        kind = rhs.kind;
        status = rhs.status;
        owned_clients = std::move(rhs.owned_clients);
        return *this;
    }

    // unsafe, caller should guarantee thread-safe.

    inline bool is_unlocked() const {
        return status == LockStatus::Unlocked;
    }

    void lock_shared(int clt) {
        kind = LockKind::Shared;
        status = LockStatus::Locked;
        owned_clients.insert(clt);
    }

    bool lock_exclusive(int clt) {
        // Ownership guard
        if (status == LockStatus::Locked && clt != *owned_clients.begin()) {
            return false;
        }
        kind = LockKind::Exclusive;
        if (status != LockStatus::Locked) {
            // add ownership for acquire first time
            owned_clients.insert(clt);
            status = LockStatus::Locked;
            return true;
        }
        return false;
    }

    void unlock_shared(int clt) {
        owned_clients.erase(clt);
        if (owned_clients.empty()) {
            status = LockStatus::Unlocked;
        }
    }

    bool unlock_exclusive(int clt) {
        // Ownership guard
        if (status == LockStatus::Locked && clt != *owned_clients.begin()) {
            return false;
        }
        status = LockStatus::Unlocked;
        owned_clients.clear();
        return true;
    }
};

struct LockCondition {
    std::condition_variable shared_cv;
    std::condition_variable exclusive_cv;
};

using condition_ptr = std::unique_ptr<LockCondition>;

class lock_server {

protected:
    int nacquire;
    std::unordered_map<lock_protocol::lockid_t, LockState> locks_store{};
    std::unordered_map<lock_protocol::lockid_t, condition_ptr> conditions_store{};
    std::mutex update_mutex;

public:
    lock_server();

    ~lock_server() {};

    lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);

    lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, LockKind kind, int &r);

    lock_protocol::status release(int clt, lock_protocol::lockid_t lid, LockKind kind, int &r);
};

#endif 