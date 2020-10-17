// lock client interface.

#ifndef lock_client_h
#define lock_client_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include <vector>

// Client interface to the lock server
class lock_client {
protected:
    rpcc *cl;
public:
    lock_client(std::string d);

    virtual ~lock_client() {};

    virtual lock_protocol::status acquire(lock_protocol::lockid_t lid, LockKind kind);

    virtual lock_protocol::status release(lock_protocol::lockid_t lid, LockKind kind);

    virtual lock_protocol::status acquire_s(lock_protocol::lockid_t lid);

    virtual lock_protocol::status release_s(lock_protocol::lockid_t lid);

    virtual lock_protocol::status acquire_e(lock_protocol::lockid_t lid);

    virtual lock_protocol::status release_e(lock_protocol::lockid_t lid);

    virtual lock_protocol::status stat(lock_protocol::lockid_t);
};

class shared_guard {
private:
    lock_client* lc;
    lock_protocol::lockid_t lid;
public:
    shared_guard(lock_client *lc, lock_protocol::lockid_t lid);
    ~shared_guard();
};

class exclusive_guard {
private:
    lock_client* lc;
    lock_protocol::lockid_t lid;
public:
    exclusive_guard(lock_client *lc, lock_protocol::lockid_t lid);

    ~exclusive_guard();
};

#endif 