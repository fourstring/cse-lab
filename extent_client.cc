// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

extent_client::extent_client(std::string dst) {
    sockaddr_in dstsock;
    make_sockaddr(dst.c_str(), &dstsock);
    cl = new rpcc(dstsock);
    if (cl->bind() != 0) {
        printf("extent_client: bind failed\n");
    }
}

extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &id) {
    extent_protocol::status ret = extent_protocol::OK;
    // Your lab2 part1 code goes here
    uint32_t inum_ret;
    cl->call(extent_protocol::create, type, inum_ret);
    id = inum_ret;
    return ret;
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf) {
    extent_protocol::status ret = extent_protocol::OK;
    // Your lab2 part1 code goes here
    std::string ret_buf{};
    cl->call(extent_protocol::get, eid, ret_buf);
    buf = std::move(ret_buf);
    return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid,
                       extent_protocol::attr &attr) {
    extent_protocol::status ret = extent_protocol::OK;
    // Your lab2 part1 code goes here
    extent_protocol::attr a{};
    cl->call(extent_protocol::getattr, eid, a);
    attr = a;
    return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf) {
    extent_protocol::status ret = extent_protocol::OK;
    // Your lab2 part1 code goes here
    int void_ret;
    cl->call(extent_protocol::put, eid, buf, void_ret);
    return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid) {
    extent_protocol::status ret = extent_protocol::OK;
    // Your lab2 part1 code goes here
    int void_ret;
    cl->call(extent_protocol::remove, eid, void_ret);
    return ret;
}

extent_protocol::status
extent_client::lookup(extent_protocol::extentid_t parent, const std::string &filename, bool &found, uint32_t &inum) {
    extent_protocol::status ret = extent_protocol::OK;
    uint32_t inum_ret;
    cl->call(extent_protocol::lookup, parent, filename, inum_ret);
    if (inum_ret != 0) {
        found = true;
        inum = inum_ret;
    } else {
        found = false;
        inum = 0;
    }
    return ret;
}

extent_protocol::status
extent_client::create_file(extent_protocol::extentid_t parent, const std::string &filename, uint32_t type,
                           uint32_t &new_inum) {
    extent_protocol::status ret = extent_protocol::OK;
    // Your lab2 part1 code goes here
    uint32_t inum_ret;
    cl->call(extent_protocol::create_file, parent, filename, type, inum_ret);
    new_inum = inum_ret;
    return ret;
}

extent_protocol::status extent_client::unlink(extent_protocol::extentid_t parent, const std::string &link_name) {
    extent_protocol::status ret = extent_protocol::OK;
    // Your lab2 part1 code goes here
    int void_ret;
    cl->call(extent_protocol::unlink, parent, link_name, void_ret);
    return ret;
}

extent_protocol::status extent_client::readdir(extent_protocol::extentid_t id, std::list<extent_dirent> &entries) {
    extent_protocol::status ret = extent_protocol::OK;
    // Your lab2 part1 code goes here
    cl->call(extent_protocol::readdir, id, entries);
    return ret;
}
