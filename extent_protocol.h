// extent wire protocol

#ifndef extent_protocol_h
#define extent_protocol_h

#include "rpc.h"

struct extent_dirent {
    std::string name;
    uint64_t inum;
};

using ull = unsigned long long;

class extent_protocol {
public:
    typedef int status;
    typedef unsigned long long extentid_t;
    enum xxstatus {
        OK, RPCERR, NOENT, IOERR
    };
    enum rpc_numbers {
        put = 0x6001,
        get,
        getattr,
        remove,
        create,
        lookup,
        create_file,
        unlink,
        readdir
    };

    enum types {
        T_DIR = 1,
        T_FILE,
        T_SYMLINK,
    };

    struct attr {
        uint32_t type;
        unsigned int atime;
        unsigned int mtime;
        unsigned int ctime;
        unsigned int size;
    };
};

inline marshall &operator<<(marshall &m, const std::list<extent_dirent> &entries) {
    m << static_cast<ull>(entries.size());
    for (const auto &ent:entries) {
        m << ent.name;
        m << static_cast<ull>(ent.inum);
    }

    return m;
}

inline unmarshall &operator>>(unmarshall &u, std::list<extent_dirent> &entries) {
    ull size;
    u >> size;
    for (ull i = 0; i < size; i++) {
        std::string name{};
        ull inum;
        u >> name >> inum;
        entries.push_back({name, inum});
    }

    return u;
}

inline unmarshall &
operator>>(unmarshall &u, extent_protocol::attr &a) {
    u >> a.type;
    u >> a.atime;
    u >> a.mtime;
    u >> a.ctime;
    u >> a.size;
    return u;
}

inline marshall &
operator<<(marshall &m, extent_protocol::attr a) {
    m << a.type;
    m << a.atime;
    m << a.mtime;
    m << a.ctime;
    m << a.size;
    return m;
}

#endif 
