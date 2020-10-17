// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>

#include <sstream>
#include <iostream>
#include <stdio.h>

lock_client::lock_client(std::string dst) {
    sockaddr_in dstsock;
    make_sockaddr(dst.c_str(), &dstsock);
    cl = new rpcc(dstsock);
    if (cl->bind() < 0) {
        printf("lock_client: call bind\n");
    }
}

int
lock_client::stat(lock_protocol::lockid_t lid) {
    int r;
    lock_protocol::status ret = cl->call(lock_protocol::stat, cl->id(), lid, r);
    VERIFY (ret == lock_protocol::OK);
    return r;
}

lock_protocol::status
lock_client::acquire(lock_protocol::lockid_t lid, LockKind kind) {
    // Your lab2 part2 code goes here
    int r;
    lock_protocol::status ret = cl->call(lock_protocol::acquire, cl->id(), lid, kind, r);
    VERIFY (ret == lock_protocol::OK);
    return r;
}

lock_protocol::status
lock_client::release(lock_protocol::lockid_t lid, LockKind kind) {
    // Your lab2 part2 code goes here
    int r;
    lock_protocol::status ret = cl->call(lock_protocol::release, cl->id(), lid, kind, r);
    VERIFY (ret == lock_protocol::OK);
    return r;
}

lock_protocol::status lock_client::acquire_s(lock_protocol::lockid_t lid) {
    return acquire(lid, LockKind::Shared);
}

lock_protocol::status lock_client::release_s(lock_protocol::lockid_t lid) {
    return release(lid, LockKind::Shared);
}

lock_protocol::status lock_client::acquire_e(lock_protocol::lockid_t lid) {
    return acquire(lid, LockKind::Exclusive);
}

lock_protocol::status lock_client::release_e(lock_protocol::lockid_t lid) {
    return release(lid, LockKind::Exclusive);
}

shared_guard::shared_guard(lock_client *lc, lock_protocol::lockid_t lid) : lc(lc), lid(lid) {
    lc->acquire_s(lid);
}

shared_guard::~shared_guard() {
    lc->release_s(lid);
}

exclusive_guard::exclusive_guard(lock_client *lc, lock_protocol::lockid_t lid) : lc(lc), lid(lid) {
    lc->acquire_e(lid);
}

exclusive_guard::~exclusive_guard() {
    lc->release_e(lid);
}
