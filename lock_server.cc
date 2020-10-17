// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server() :
        nacquire(0) {
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r) {
    lock_protocol::status ret = lock_protocol::OK;
    printf("stat request from clt %d\n", clt);
    r = nacquire;
    return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, LockKind kind, int &r) {
    lock_protocol::status ret = lock_protocol::OK;
    // Your lab2 part2 code goes here
    std::unique_lock<std::mutex> guard{update_mutex};
    auto ilock_state = locks_store.find(lid);
    if (ilock_state != locks_store.end()) {
        // check exist lock state
        auto &lock_state = ilock_state->second;
        auto try_lock_with_compatibility = [&lock_state, kind, clt]() -> bool {
            auto existed_kind = lock_state.kind;
            if (lock_state.is_unlocked()) {
                if (kind == LockKind::Shared) {
                    lock_state.lock_shared(clt);
                } else {
                    lock_state.lock_exclusive(clt);
                }
                return true;
            }
            if (existed_kind == LockKind::Shared && kind == LockKind::Shared) {
                lock_state.lock_shared(clt);
                return true;
            } else if (existed_kind != kind) {
                return false;
            } else {
                return lock_state.lock_exclusive(clt);
            }
            return false;
        };

        if (try_lock_with_compatibility()) {
            r = true;
            return ret;
        }
        auto &condition = conditions_store.find(lid)->second;
        while (!try_lock_with_compatibility()) {
            // Insert lock state and condition at the same time is guaranteed
            if (kind == LockKind::Shared) {
                condition->shared_cv.wait(guard);
            } else {
                condition->exclusive_cv.wait(guard);
            }
        }
        r = true;
        return ret;
    } else {
        // create new state and lock it
        LockState state{kind};
        if (kind == LockKind::Shared) {
            state.lock_shared(clt);
        } else {
            state.lock_exclusive(clt);
        }
        locks_store[lid] = std::move(state);
        conditions_store[lid] = condition_ptr(new LockCondition{});
    }
    guard.unlock();
    return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, LockKind kind, int &r) {
    lock_protocol::status ret = lock_protocol::OK;
    // Your lab2 part2 code goes here
    std::unique_lock<std::mutex> guard{update_mutex};
    auto ilock_state = locks_store.find(lid);
    if (ilock_state == locks_store.end()) {
        r = true;
        return ret;
    }
    auto &lock_state = ilock_state->second;
    if (kind == LockKind::Shared) {
        lock_state.unlock_shared(clt);
    } else {
        lock_state.unlock_exclusive(clt);
    }
    if (lock_state.is_unlocked()) {
        guard.unlock();
        auto &condition = conditions_store.find(lid)->second;
        // Maybe potential starvation
        condition->shared_cv.notify_all();
        condition->exclusive_cv.notify_one();
        r = true;
        return ret;
    }
    r = true;
    guard.unlock();
    return ret;
}