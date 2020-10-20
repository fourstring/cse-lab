#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>
#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"
#include "handle.h"
#include <condition_variable>
#include <mutex>
#include <memory>
#include <queue>
#include "concurrent_queue.h"
#include <thread>

enum class RLockStatus {
    Granted,
    Free,
    Revoking,
    Retrying
};


struct RevokeTask {
    lock_protocol::lockid_t lid{};
    std::string requester_handle;

    RevokeTask(unsigned long long int lid, std::string requesterHandle);

    RevokeTask(RevokeTask &&rhs) noexcept;

    RevokeTask(RevokeTask &rhs);
};

using thread_ptr = std::unique_ptr<std::thread>;

struct RLockState {
    RLockStatus status;
    std::string owned_handle;
    concurrent_queue<RevokeTask> revoke_queue;
    bool _stop = false;

    RLockState(RLockStatus status, const std::string &ownedHandle);

    RLockState(RLockState &&rhs) noexcept;

    thread_ptr revoker{};

    void revoke_handler();

    void start();

    void stop();
};


using lock_store_t = std::map<lock_protocol::lockid_t, RLockState>;


class lock_server_cache {
private:
    int nacquire{};
    lock_store_t lock_store;
    std::mutex access_lock_mutex;
    concurrent_queue<RevokeTask> revoke_queue;
    bool _stop = false;
public:
    lock_server_cache();

    lock_protocol::status stat(lock_protocol::lockid_t, int &);

    int acquire(lock_protocol::lockid_t lid, std::string &id, int &result);

    int release(lock_protocol::lockid_t lid, std::string &id, int &result);

    void revoker();
};

#endif
