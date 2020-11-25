#include "ydb_client.h"

//#define DEBUG 1

ydb_client::ydb_client(const std::string &dst) : current_transaction(INVALID_TRANSID) {
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
	if (this->current_transaction != INVALID_TRANSID) {
		throw ydb_protocol::TRANSIDINV;
	}
	ydb_protocol::status ret = cl->call(ydb_protocol::transaction_begin, a, transid);
	if (ret != ydb_protocol::OK) {
		this->current_transaction = INVALID_TRANSID;
		throw ret;
	}
	this->current_transaction = transid;
}

void ydb_client::transaction_commit() {
	int r;
	ydb_protocol::status ret = cl->call(ydb_protocol::transaction_commit, this->current_transaction, r);
	this->current_transaction = INVALID_TRANSID;
	if (ret != ydb_protocol::OK) {
		throw ret;
	}
}

void ydb_client::transaction_abort() {
	int r;
	ydb_protocol::status ret = cl->call(ydb_protocol::transaction_abort, this->current_transaction, r);
	this->current_transaction = INVALID_TRANSID;
	if (ret != ydb_protocol::OK) {
		throw ret;
	}
}

std::string ydb_client::get(const std::string &key) {
	std::string value;
	ydb_protocol::status ret = cl->call(ydb_protocol::get, this->current_transaction, key, value);
	if (ret != ydb_protocol::OK) {
		this->current_transaction = INVALID_TRANSID;
		throw ret;
	}
	return value;
}

void ydb_client::set(const std::string &key, const std::string &value) {
	int r;
	ydb_protocol::status ret = cl->call(ydb_protocol::set, this->current_transaction, key, value, r);
	if (ret != ydb_protocol::OK) {
		this->current_transaction = INVALID_TRANSID;
		throw ret;
	}
}

void ydb_client::del(const std::string &key) {
	int r;
	ydb_protocol::status ret = cl->call(ydb_protocol::del, this->current_transaction, key, r);
	if (ret != ydb_protocol::OK) {
		this->current_transaction = INVALID_TRANSID;
		throw ret;
	}
}

