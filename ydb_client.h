#ifndef ydb_client_h
#define ydb_client_h

#include <string>
#include "ydb_protocol.h"
#include "rpc.h"


class ydb_client {
private:
#define INVALID_TRANSID (-1)
	rpcc *cl;
	ydb_protocol::transaction_id currect_transaction;
public:
	ydb_client(const std::string &);
	~ydb_client();
	void transaction_begin();
	void transaction_commit();
	void transaction_abort();
	std::string get(const std::string &);
	void set(const std::string &, const std::string &);
	void del(const std::string &);
};

#endif

