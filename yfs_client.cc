// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client(extent_dst);
    // Lab2: Use lock_client_cache when you test lock_cache
    lc = new lock_client(lock_dst);
    // lc = new lock_client_cache(lock_dst);
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
}

yfs_client::inum
yfs_client::n2i(std::string n) {
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum) {
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum) {
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    }
    printf("isfile: %lld is a dir\n", inum);
    return false;
}

/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool
yfs_client::isdir(inum inum) {
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    }
    printf("isdir: %lld is not a dir\n", inum);
    return false;
}

bool
yfs_client::issymlink(inum inum) {
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_SYMLINK) {
        printf("isfile: %lld is a symlink\n", inum);
        return true;
    }
    printf("isfile: %lld is not a symlink\n", inum);
    return false;
}


int
yfs_client::getfile(inum inum, fileinfo &fin) {
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

    release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din) {
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

    release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size) {
    int r = OK;

    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    extent_protocol::attr a;
    if (ec->getattr(ino, a) != extent_protocol::OK) {
        return IOERR;
    }

    if (size == a.size) {
        return OK;
    }


    if (size < a.size) {
        // do truncate
        auto file_buf = std::string(a.size, '\0');
        ec->get(ino, file_buf);
        auto truncated = file_buf.substr(0, size);
        ec->put(ino, truncated);
    } else {
        // size must > a.size
        auto file_buf = std::string(size, '\0');
        ec->get(ino, file_buf);
        ec->put(ino, file_buf);
    }

    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out) {
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    auto found = false;
    auto ino = inum{0};
    ec->lookup(parent, name, found, reinterpret_cast<uint32_t &>(ino));

    if (found) {
        return EXIST;
    }

    ec->create_file(parent, name, extent_protocol::T_FILE, reinterpret_cast<uint32_t &>(ino));

    ino_out = ino;
    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out) {
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */

    auto found = false;
    auto ino = inum{0};
    ec->lookup(parent, name, found, reinterpret_cast<uint32_t &>(ino));

    if (found) {
        return EXIST;
    }

    ec->create_file(parent, name, extent_protocol::T_DIR, reinterpret_cast<uint32_t &>(ino));

    ino_out = ino;
    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out) {
    int r = OK;

    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    ec->lookup(parent, name, found, reinterpret_cast<uint32_t &>(ino_out));
    /*auto parent_dir = ec->get_dir(parent);
    auto file_inum = parent_dir.filename_to_inum(name);

    if (file_inum != 0) {
        found = true;
        ino_out = file_inum;
    } else {
        found = false;
        ino_out = 0;
    }*/

    return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list) {
    int r = OK;

    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    auto entries = std::list<extent_dirent>{};
    ec->readdir(dir, entries);
    for (const auto &ent:entries) {
        list.push_back(dirent{ent.name, ent.inum});
    }

    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data) {
    int r = OK;

    /*
     * your code goes here.
     * note: read using ec->get().
     */
    extent_protocol::attr a;
    ec->getattr(ino, a);
    if (a.type == 0) {
        return IOERR;
    }

    auto buf = std::string{};
    ec->get(ino, buf);
    data = buf.substr(off, size);

    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
                  size_t &bytes_written) {
    int r = OK;

    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    extent_protocol::attr a;
    ec->getattr(ino, a);
    if (a.type == 0) {
        return IOERR;
    }

    bytes_written = 0;

    auto least_new_size = (off + 1) + (size - 1);
    auto buf = std::string{};
    ec->get(ino, buf);
    auto new_buf = std::string(least_new_size > a.size ? least_new_size : a.size, '\0');
    std::copy(buf.begin(), buf.end(), new_buf.begin());
    for (int i = 0; i < size; ++i) {
        new_buf[off + i] = data[i];
        bytes_written++;
    }
    ec->put(ino, new_buf);

    return r;
}

int yfs_client::unlink(inum parent, const char *name) {
    int r = OK;

    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    ec->unlink(parent, name);

    return r;
}

int yfs_client::symlink(yfs_client::inum parent, const char *link, const char *name, inum &ino_out) {
    int r = OK;

    auto found = false;
    auto ino = inum{0};
    ec->lookup(parent, name, found, reinterpret_cast<uint32_t &>(ino));

    if (found) {
        return EXIST;
    }

    ec->create_file(parent, name, extent_protocol::T_SYMLINK, reinterpret_cast<uint32_t &>(ino));

    ino_out = ino;
    auto size = strlen(link);
    r = write(ino, size, 0, link, size);
    return r;
}

