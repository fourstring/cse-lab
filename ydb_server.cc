#include "ydb_server.h"
#include "extent_client.h"
#include "tprintf.h"

//#define DEBUG 1

static long timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

ydb_server::ydb_server(std::string extent_dst, std::string lock_dst) {
    ec = new extent_client(extent_dst);
    lc = new lock_client(lock_dst);
    //lc = new lock_client_cache(lock_dst);

    long starttime = timestamp();

    for (int i = 2; i < 1024; i++) {    // for simplicity, just pre alloc all the needed inodes
        extent_protocol::extentid_t id;
        ec->create(extent_protocol::T_FILE, id);
    }

    long endtime = timestamp();
    printf("time %ld ms\n", endtime - starttime);
}

ydb_server::~ydb_server() {
    delete lc;
    delete ec;
}


ydb_protocol::status ydb_server::transaction_begin(int,
                                                   ydb_protocol::transaction_id &out_id) {
    out_id = next_tid.fetch_add(1);
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::transaction_commit(ydb_protocol::transaction_id id, int &) {
    // no imply, just return OK
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::transaction_abort(ydb_protocol::transaction_id id, int &) {
    // no imply, just return OK
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::get(ydb_protocol::transaction_id tid, std::string &key, std::string &ret) {
    // lab3: your code here
    auto inum = hasher(key) % 1024;
    ec->get(inum, ret);
    return ydb_protocol::OK;
}

ydb_protocol::status
ydb_server::set(ydb_protocol::transaction_id tid, std::string &key, std::string &value, int &) {
    // lab3: your code here
    auto inum = hasher(key) % 1024;
    ec->put(inum, value);
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::del(ydb_protocol::transaction_id tid, std::string &key, int &) {
    // lab3: your code here
    int t;

    return set(tid, key, empty, t);
}

ydb_server::ydb_server() = default;

