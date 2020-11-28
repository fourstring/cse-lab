#ifndef ydb_server_occ_h
#define ydb_server_occ_h

#include <string>
#include <map>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include "extent_client.h"
#include "lock_protocol.h"
#include "lock_client.h"
#include "lock_client_cache.h"
#include "ydb_protocol.h"
#include "ydb_server.h"

using versions_t = std::unordered_map<lock_protocol::lockid_t, ydb_protocol::transaction_id>;
using write_set_t = std::unordered_map<lock_protocol::lockid_t, std::string>;

struct ReadVal {
    std::string val{""};
    int version = 0;

    ReadVal(const std::string &val, int version);
};

using read_set_t = std::unordered_map<lock_protocol::lockid_t, ReadVal>;
struct OCCTransaction {
    read_set_t read_set;
    write_set_t write_set;
};
using occ_trans_set_t = std::unordered_map<ydb_protocol::transaction_id, OCCTransaction>;

class ydb_server_occ : public ydb_server {
private:
    std::mutex validate_mutex;
    versions_t versions;
    occ_trans_set_t transactions;
public:


    ydb_server_occ(std::string, std::string);

    ~ydb_server_occ();

    ydb_protocol::status transaction_begin(int, ydb_protocol::transaction_id &);

    ydb_protocol::status transaction_commit(ydb_protocol::transaction_id, int &);

    ydb_protocol::status transaction_abort(ydb_protocol::transaction_id, int &);

    void clean_transaction(ydb_protocol::transaction_id tid);

    ydb_protocol::status get(ydb_protocol::transaction_id, std::string &key, std::string &);

    ydb_protocol::status set(ydb_protocol::transaction_id tid, std::string &key, std::string &value, int &);

    ydb_protocol::status del(ydb_protocol::transaction_id tid, std::string &key, int &);
};

#endif

