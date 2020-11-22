// ydb wire protocol

#ifndef ydb_protocol_h
#define ydb_protocol_h

#include "rpc.h"

class ydb_protocol {
public:
	typedef int status;
	enum xxstatus { OK = 0, RPCERR, ABORT, TRANSIDINV };
	
	typedef int transaction_id;
	
	enum rpc_numbers {
		transaction_begin = 0x9001,
		transaction_commit,
		transaction_abort,
		get,
		set,
		del,
	};
};

#endif 

