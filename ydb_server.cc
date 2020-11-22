#include "ydb_server.h"
#include "extent_client.h"

//#define DEBUG 1

ydb_server::ydb_server(std::string extent_dst, std::string lock_dst) {
	ec = new extent_client(extent_dst);
	lc = new lock_client(lock_dst);
	//lc = new lock_client_cache(lock_dst);

	for(int i = 2; i < 1024; i++) {    // for simplicity, just pre alloc all the needed inodes
		extent_protocol::extentid_t id;
		ec->create(extent_protocol::T_FILE, id);
	}
}

ydb_server::~ydb_server() {
	delete lc;
	delete ec;
}


ydb_protocol::status ydb_server::transaction_begin(int, ydb_protocol::transaction_id &out_id) {    // the first arg is not used, it is just a hack to the rpc lib
	// no imply, just return OK
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

ydb_protocol::status ydb_server::get(ydb_protocol::transaction_id id, const std::string key, std::string &out_value) {
	// lab3: your code here
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::set(ydb_protocol::transaction_id id, const std::string key, const std::string value, int &) {
	// lab3: your code here
	return ydb_protocol::OK;
}

ydb_protocol::status ydb_server::del(ydb_protocol::transaction_id id, const std::string key, int &) {
	// lab3: your code here
	return ydb_protocol::OK;
}

