// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst,
                                     class lock_release_user *_lu)
        : lock_client(xdst), lu(_lu) {
    srand(time(NULL) ^ last_port);
    rlock_port = ((rand() % 32000) | (0x1 << 10));
    const char *hname;
    // VERIFY(gethostname(hname, 100) == 0);
    hname = "127.0.0.1";
    std::ostringstream host;
    host << hname << ":" << rlock_port;
    id = host.str();
    last_port = rlock_port;
    rpcs *rlsrpc = new rpcs(rlock_port);
    rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
    rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid, LockKind kind = LockKind::Exclusive) {
    int ret = lock_protocol::OK;
    tprintf("%s: acquiring lock %lld\n", id.data(), lid)
    // Your lab2 part3 code goes here
    {
        unique_t u{update_mutex};
        auto istatus = status.find(lid);
        auto &condition = conditions[lid];
        if (istatus != status.end()) {
            if (istatus->second != RLockStatus::None) {
                // Local handling logic
                auto try_lock = [&condition, &istatus]() -> bool {
                    if (istatus->second == RLockStatus::Free) {
                        istatus->second = RLockStatus::Locked;
                        // TODO: Add client side waiting mechanism
                        return true;
                    } else {
                        return false;
                    }
                };
                while (!try_lock()) {
                    condition->acquire_cv.wait(u);
                }
                tprintf("%s: acquired lock %lld locally\n", id.data(), lid)
                return ret;
            } else {
                istatus->second = RLockStatus::ToAcquire;
            }
        } else {
            status.insert({lid, RLockStatus::ToAcquire});
            conditions[lid] = rcondition_ptr{new RLockCondition{}};
        }
    }
    // local has no lock, acquire from server
    int acquire_ret;
    tprintf("%s: start fetching lock %lld from server\n", id.data(), lid)
    cl->call(lock_protocol::acquire, lid, id, acquire_ret);
    bool respond_retry = false;
    {
        unique_t u{update_mutex};
        auto &condition = conditions[lid];
        if (acquire_ret == lock_protocol::OK) {
            tprintf("%s: acquired lock %lld from remote directly\n", id.data(), lid)
            status[lid] = RLockStatus::Locked;
            return ret;
        } else if (acquire_ret == lock_protocol::RETRY) {
            if (condition->pending_retry) {
                tprintf("%s: acquired lock %lld from pending retry\n", id.data(), lid)
                status[lid] = RLockStatus::Locked;
                respond_retry = true;
                condition->pending_retry = false;
            } else {
                status[lid] = RLockStatus::Acquiring;
                condition->fetching_cv.wait(u);
                tprintf("%s: acquired lock %lld from later retry\n", id.data(), lid)
                status[lid] = RLockStatus::Locked;
            }
        }
        if (respond_retry) {
            condition->retry_response.notify_all();
        }
    }

    return ret;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid, LockKind kind = LockKind::Exclusive) {
    int ret = lock_protocol::OK;
    // Your lab2 part3 code goes here
    bool respond_revoke = false;
    {
        unique_t u{update_mutex};
        auto &condition = conditions[lid];
        // give lock back takes advantage
        if (condition->pending_revoke) {
            tprintf("%s: give lock back %lld for pending revoke\n", id.data(), lid)
            respond_revoke = true;
            condition->pending_revoke = false;
            status[lid] = RLockStatus::None;
        } else {
            tprintf("%s: release lock %lld locally\n", id.data(), lid)
            status[lid] = RLockStatus::Free;
        }
        if (respond_revoke) {
            condition->revoke_response.notify_all();
        } else {
            condition->acquire_cv.notify_one();
        }
    }

    return ret;
}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid,
                                  int &) {
    int ret = rlock_protocol::OK;
    // Your lab2 part3 code goes here
    tprintf("%s: received revoke request for lock %lld\n", id.data(), lid)
    {
        unique_t u{update_mutex};
        auto &stat = status[lid];
        auto &condition = conditions[lid];
        if (stat != RLockStatus::Free) {
            tprintf("%s: registered pending revoke for lock %lld\n", id.data(), lid)
            condition->pending_revoke = true;
            condition->revoke_response.wait(u);
        } else {
            // TODO: add logic on state None, in which client has no lock
            tprintf("%s: revoke lock %lld by handler directly\n", id.data(), lid)
            status[lid] = RLockStatus::None;
        }
    }
    return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid,
                                 int &) {
    int ret = rlock_protocol::OK;
    // Your lab2 part3 code goes here
    tprintf("%s: received retry request for lock %lld\n", id.data(), lid)
    {
        unique_t u{update_mutex};
        auto &stat = status[lid];
        auto &condition = conditions[lid];
        if (stat != RLockStatus::Acquiring) {
            tprintf("%s: registered pending retry for lock %lld\n", id.data(), lid)
            condition->pending_retry = true;
            condition->retry_response.wait(u);
        } else if (stat == RLockStatus::Acquiring) {
            tprintf("%s: notified retry for acquiring lock %lld\n", id.data(), lid)
            condition->fetching_cv.notify_all();
        }
    }
    return ret;
}

void lock_client_cache::free_revoker() {
    while (!_stop) {
        auto lid = revoke_queue.dequeue();
        {
            unique_t u{update_mutex};
            auto &stat = status[lid];
            auto &condition = conditions[lid];

        }
    }
}
