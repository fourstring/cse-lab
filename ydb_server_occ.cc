#include "ydb_server_occ.h"
#include "extent_client.h"
#include "tprintf.h"

//#define DEBUG 1

static long timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

ydb_server_occ::ydb_server_occ(std::string extent_dst, std::string lock_dst) {
    ec = new extent_client(extent_dst);
    lc = new lock_client(lock_dst);
    //lc = new lock_client_cache(lock_dst);

    long starttime = timestamp();

    for (int i = 2; i < 1024; i++) {    // for simplicity, just pre alloc all the needed inodes
        extent_protocol::extentid_t id;
        ec->create(extent_protocol::T_FILE, id);
        versions[i] = 0;
    }

    long endtime = timestamp();
    printf("time %ld ms\n", endtime - starttime);
}

ydb_server_occ::~ydb_server_occ() {
}


ydb_protocol::status ydb_server_occ::transaction_begin(int,
                                                       ydb_protocol::transaction_id &out_id) {    // the first arg is not used, it is just a hack to the rpc lib
    // lab3: your code here
    int _ = 0;
    ydb_protocol::transaction_id tid;
    ydb_server::transaction_begin(_, tid);
    tprintf("Transaction %d begin\n", tid)
    unique_t u{validate_mutex};
    {
        auto[e, ok] = transactions.try_emplace(tid);
        if (!ok) {
            return ydb_protocol::TRANSIDINV;
        }
    }
    out_id = tid;
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_occ::transaction_commit(ydb_protocol::transaction_id id, int &) {
    // lab3: your code here
    unique_t u{validate_mutex};
    auto &trans = transactions[id];
    for (auto &[lid, rv]:trans.read_set) {
        auto &[val, ver]=rv;
        if (ver == versions[lid]) {

        } else {
            tprintf("Transaction %d conflict on %llu, read ver %d but now ver %d\n", id, lid, ver, versions[lid])
            clean_transaction(id);
            return ydb_protocol::ABORT;
        }
    }
    for (auto &[lid, val]:trans.write_set) {
        ec->put(lid, val);
        versions[lid]++;
    }
    tprintf("Transaction %d committed\n", id)
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_occ::transaction_abort(ydb_protocol::transaction_id id, int &) {
    // lab3: your code here
    unique_t u{validate_mutex};
    clean_transaction(id);
    tprintf("Transaction %d aborted\n", id)
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_occ::get(ydb_protocol::transaction_id id, std::string &key, std::string &ret) {
    // lab3: your code here
    auto itrans = transactions.find(id);
    if (itrans == transactions.end()) {
        return ydb_protocol::TRANSIDINV;
    }
    auto &trans = itrans->second;
    auto lid = hasher(key) % 1024;
//    tprintf("Transaction %d start to get %s, hashed as %lu\n", id, key.data(), lid)
    try {
        ret = trans.write_set.at(lid);
        return ydb_protocol::OK;
    } catch (std::out_of_range &e) {

    }
    try {
        ret = trans.read_set.at(lid).val;
        return ydb_protocol::OK;
    } catch (std::out_of_range &e) {

    }
    auto version = 0;
    {
        unique_t u{validate_mutex};
        version = versions[lid];
        ec->get(lid, ret);
    }

    trans.read_set.try_emplace(lid, ret, version);
    //tprintf("Transaction %d has got %s, hashed as %lu, successfully: %s\n", id, key.data(), lid, ret.data())
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_occ::set(ydb_protocol::transaction_id id, std::string &key, std::string &value, int &) {
    // lab3: your code here
    auto itrans = transactions.find(id);
    if (itrans == transactions.end()) {
        return ydb_protocol::TRANSIDINV;
    }
    auto &trans = itrans->second;
    auto lid = hasher(key) % 1024;
    //tprintf("Transaction %d start to set %s, hashed as %lu\n", id, key.data(), lid)
    trans.write_set[lid] = value;
    //tprintf("Transaction %d has set %s, hashed as %lu, successfully: %s\n", id, key.data(), lid, value.data())
    return ydb_protocol::OK;
}

ydb_protocol::status ydb_server_occ::del(ydb_protocol::transaction_id id, std::string &key, int &) {
    // lab3: your code here
    int _;
    return set(id, key, empty, _);
}

void ydb_server_occ::clean_transaction(ydb_protocol::transaction_id tid) {
    transactions.erase(tid);
}

ReadVal::ReadVal(const std::string &val, int version) : val(val), version(version) {}
