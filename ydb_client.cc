#include "ydb_client.h"

//#define DEBUG 1

ydb_client::ydb_client(const std::string &dst) : currect_transaction(INVALID_TRANSID) {
	sockaddr_in dstsock;
	make_sockaddr(dst.c_str(), &dstsock);
	cl = new rpcc(dstsock);
	if (cl->bind() < 0) {
		printf("ydb_client: call bind error\n");
	}
}

ydb_client::~ydb_client() {
	delete cl;
}

void ydb_client::transaction_begin() {
	int a;
	ydb_protocol::transaction_id transid;
	if (this->currect_transaction != INVALID_TRANSID) {
		throw ydb_protocol::TRANSIDINV;
	}
	ydb_protocol::status ret = cl->call(ydb_protocol::transaction_begin, a, transid);
	if (ret != ydb_protocol::OK) {
		this->currect_transaction = INVALID_TRANSID;
		throw ret;
	}
	this->currect_transaction = transid;
}

void ydb_client::transaction_commit() {
	int r;
	ydb_protocol::status ret = cl->call(ydb_protocol::transaction_commit, this->currect_transaction, r);
	this->currect_transaction = INVALID_TRANSID;
	if (ret != ydb_protocol::OK) {
		throw ret;
	}
}

void ydb_client::transaction_abort() {
	int r;
	ydb_protocol::status ret = cl->call(ydb_protocol::transaction_abort, this->currect_transaction, r);
	this->currect_transaction = INVALID_TRANSID;
	if (ret != ydb_protocol::OK) {
		throw ret;
	}
}

std::string ydb_client::get(const std::string &key) {
	std::string value;
	ydb_protocol::status ret = cl->call(ydb_protocol::get, this->currect_transaction, key, value);
	if (ret != ydb_protocol::OK) {
		this->currect_transaction = INVALID_TRANSID;
		throw ret;
	}
	return value;
}

void ydb_client::set(const std::string &key, const std::string &value) {
	int r;
	ydb_protocol::status ret = cl->call(ydb_protocol::set, this->currect_transaction, key, value, r);
	if (ret != ydb_protocol::OK) {
		this->currect_transaction = INVALID_TRANSID;
		throw ret;
	}
}

void ydb_client::del(const std::string &key) {
	int r;
	ydb_protocol::status ret = cl->call(ydb_protocol::del, this->currect_transaction, key, r);
	if (ret != ydb_protocol::OK) {
		this->currect_transaction = INVALID_TRANSID;
		throw ret;
	}
}

