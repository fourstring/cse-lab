// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() {
    im = new inode_manager();
}

int extent_server::create(uint32_t type, extent_protocol::extentid_t &id) {
    // alloc a new inode and return inum
    printf("extent_server: create inode\n");
    id = im->alloc_inode(type);

    return extent_protocol::OK;
}

int extent_server::put(extent_protocol::extentid_t id, std::string &buf, int &) {
    id &= 0x7fffffff;

    const char *cbuf = buf.c_str();
    int size = buf.size();
    im->write_file(id, cbuf, size);

    return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf) {
    printf("extent_server: get %lld\n", id);

    id &= 0x7fffffff;

    int size = 0;
    char *cbuf = NULL;

    im->read_file(id, &cbuf, &size);
    if (size == 0)
        buf = "";
    else {
        buf.assign(cbuf, size);
        free(cbuf);
    }

    return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a) {
    printf("extent_server: getattr %lld\n", id);

    id &= 0x7fffffff;

    extent_protocol::attr attr;
    memset(&attr, 0, sizeof(attr));
    im->getattr(id, attr);
    a = attr;

    return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &) {
    printf("extent_server: write %lld\n", id);

    id &= 0x7fffffff;
    im->remove_file(id);

    return extent_protocol::OK;
}

Directory extent_server::get_dir(extent_protocol::extentid_t id) {
    return Directory{im, static_cast<uint32_t>(id)};
}

int
extent_server::lookup(extent_protocol::extentid_t parent, std::string &filename, uint32_t &inum) {
    auto dir = get_dir(parent);
    auto file_inum = dir.filename_to_inum(filename);
    inum = file_inum;
    return extent_protocol::OK;
}

int extent_server::create_file(extent_protocol::extentid_t parent, std::string &filename, uint32_t type,
                               uint32_t &new_inum) {
    auto dir = get_dir(parent);
    new_inum = dir.create_file(filename, type);
    return extent_protocol::OK;
}

int extent_server::unlink(extent_protocol::extentid_t parent, std::string &link_name, int &void_ret) {
    auto dir = get_dir(parent);
    dir.unlink(link_name);
    return extent_protocol::OK;
}

int extent_server::readdir(extent_protocol::extentid_t id, std::list<extent_dirent> &entries) {
    auto dir = get_dir(id);
    for (auto entry = dir.cbegin(); entry != dir.cend(); entry++) {
        entries.push_back({entry->first, entry->second});
    }
    return extent_protocol::OK;
}

