// lock protocol

#ifndef lock_protocol_h
#define lock_protocol_h

#include "rpc.h"

enum class LockStatus {
    Locked,
    Unlocked
};

enum class LockKind {
    Shared,
    Exclusive,
    ForTest
};

inline marshall &operator<<(marshall &m, LockKind kind) {
    m << static_cast<int >(kind);
    return m;
}

inline marshall &operator<<(marshall &m, LockStatus status) {
    m << static_cast<int >(status);
    return m;
}

inline unmarshall &operator>>(unmarshall &u, LockKind &kind) {
    int i;
    u >> i;
    kind = static_cast<LockKind>(i);
    return u;
}

inline unmarshall &operator>>(unmarshall &u, LockStatus &status) {
    int i;
    u >> i;
    status = static_cast<LockStatus>(i);
    return u;
}

class lock_protocol {
public:
    enum xxstatus {
        OK, RETRY, RPCERR, NOENT, IOERR
    };
    typedef int status;
    typedef unsigned long long lockid_t;
    enum rpc_numbers {
        acquire = 0x7001,
        release,
        stat
    };
};

class rlock_protocol {
public:
    enum xxstatus {
        OK, RPCERR
    };
    typedef int status;
    enum rpc_numbers {
        revoke = 0x8001,
        retry = 0x8002
    };
};


#endif 