#ifndef yfs_client_h
#define yfs_client_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "lock_client_cache.h"
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>


class yfs_client {
    extent_client *ec;
    lock_client *lc;
public:

    typedef unsigned long long inum;
    enum xxstatus {
        OK, RPCERR, NOENT, IOERR, EXIST
    };
    typedef int status;

    struct fileinfo {
        uint32_t type;
        unsigned long long size;
        unsigned long atime;
        unsigned long mtime;
        unsigned long ctime;
    };
    struct dirinfo {
        unsigned long atime;
        unsigned long mtime;
        unsigned long ctime;
    };
    struct dirent {
        std::string name;
        yfs_client::inum inum;
    };

private:
    static std::string filename(inum);

    static inum n2i(std::string);

public:
    yfs_client(std::string, std::string);

    bool isfile(inum);

    bool isdir(inum);

    int getfile(inum, fileinfo &);

    int getdir(inum, dirinfo &);

    int setattr(inum, size_t);

    int _lookup(inum parent, const char *name, bool &found, inum &ino_out);

    int lookup(inum parent, const char *name, bool &found, inum &ino_out);

    int create(inum, const char *, mode_t, inum &);

    int readdir(inum, std::list<dirent> &);

    int write(inum, size_t, off_t, const char *, size_t &);

    int _write(inum ino, size_t size, off_t off, const char *data,
               size_t &bytes_written);

    int read(inum ino, size_t size, off_t off, std::string &data);

    int unlink(inum, const char *);

    int mkdir(inum, const char *, mode_t, inum &);

    /** you may need to add symbolic link related methods here.*/

    int symlink(inum parent, const char *link, const char *name, inum &ino_out);

    bool issymlink(inum inum);
};

#endif 
