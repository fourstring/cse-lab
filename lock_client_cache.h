// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
#include <map>
#include <condition_variable>
#include <mutex>
#include <memory>
#include "concurrent_queue.h"

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 6.
class lock_release_user {
public:
    virtual void dorelease(lock_protocol::lockid_t) = 0;

    virtual ~lock_release_user() {};
};

enum class RLockStatus {
    None,
    Locked,
    Free,
    ToAcquire,
    Acquiring,
    Releasing
};

struct RLockCondition {
    std::condition_variable acquire_cv; // For notification of granting and releasing logic locally
    std::condition_variable fetching_cv; // For notification of fetching lock from server
    std::condition_variable revoke_response; // For continue response of revoke requests from server
    std::condition_variable retry_response; // For continue response of retry requests from server
    bool pending_revoke; // Flag for there is a pending revoke request, should be responded by revoke_response cv
    bool pending_retry; // Flag for there is a pending retry request, should be responded by retry_response cv
};


class lock_client_cache : public lock_client {
private:
    using status_map_t = std::map<lock_protocol::lockid_t, RLockStatus>;
    using rcondition_ptr = std::unique_ptr<RLockCondition>;
    using conditions_map_t = std::map<lock_protocol::lockid_t, rcondition_ptr>;

    class lock_release_user *lu;

    int rlock_port;
    std::string hostname;
    std::string id;

    status_map_t status{};
    conditions_map_t conditions{};
    std::mutex update_mutex;
    concurrent_queue<lock_protocol::lockid_t> revoke_queue;
    bool _stop = false;
public:
    static int last_port;

    lock_client_cache(std::string xdst, class lock_release_user *l = 0);

    virtual ~lock_client_cache() {};

    lock_protocol::status acquire(lock_protocol::lockid_t, LockKind kind) override;

    lock_protocol::status release(lock_protocol::lockid_t, LockKind kind) override;

    rlock_protocol::status revoke_handler(lock_protocol::lockid_t,
                                          int &);

    rlock_protocol::status retry_handler(lock_protocol::lockid_t,
                                         int &);

    void free_revoker();
};


#endif
