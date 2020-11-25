// basic test for 2PL and OCC: single thread, serializable transactions, fault handling

#include <iostream>
#include <cstdio>
#include "ydb_client.h"

#define CHECK(express, errmsg) do { \
	if (!(express)) { \
		printf("Error%d : '%s' at %s:%d : %s \n", ++global_err_count, #express, __FILE__, __LINE__, errmsg); \
	} \
} while(0)

#define CHECK_TRANS_EXCEPTION(express, expected_exception, errmsg) do { \
	bool exception_occurred = false; \
	try { \
		express; \
	} \
	catch(ydb_protocol::status e) { \
		exception_occurred = (e == expected_exception); \
	} \
		CHECK(exception_occurred, errmsg); \
} while(0)

int global_err_count = 0;

int test_transaction_functionality(ydb_client &y) {
	std::string r;
	y.transaction_begin();
	r = y.get("a");
		// 1. read committed value
		CHECK(r == "100", "read initial committed value error");
	y.set("a", "300");
	r = y.get("a");
		// 2. read self modified but not committed value
		CHECK(r == "300", "read self modified value error");
	r = y.get("a");
		// 3. repeatable read
		CHECK(r == "300", "repeatable read value error");
	y.set("b", "210");
	y.transaction_commit();
	
	y.transaction_begin();
	r = y.get("a");
		// 4. read committed value
		CHECK(r == "300", "read committed value error");
	r = y.get("b");
		// 5. read committed value
		CHECK(r == "210", "read committed value error");
	y.transaction_commit();
	return 0;
}

int test_transaction_exception(ydb_client &y) {
	std::string r;
	
	y.transaction_begin();
	y.set("b", "400");
	r = y.get("b");
		// 6. read committed value
		CHECK(r == "400", "read self modified committed value error");
	// manully call transaction abort, so it should return OK rather than ABORT
	y.transaction_abort();

	// 7. check handling of invalid transaction
	CHECK_TRANS_EXCEPTION(y.get("a"), ydb_protocol::TRANSIDINV, "invalid transaction check error");

	y.transaction_begin();
	r = y.get("b");
		// 8. aborted transaction should not change origin value
		CHECK(r == "210", "aborted transaction should not change origin value");
	y.transaction_commit();

	return 0;
}

int main(int argc, char **argv) {
	if (argc <= 1) {
		printf("Usage: %s <ydb_server_listen_port>\n", argv[0]);
		return 0;
	}

	std::string r;
	ydb_client y((std::string(argv[1])));

	y.transaction_begin();
	y.set("a", "100");
	y.set("b", "200");
	y.transaction_commit();

	test_transaction_functionality(y);
	test_transaction_exception(y);

	if (global_err_count != 0) {
		printf("[x_x] Fail test-lab3-part2-3-basic\n");
	}
	else {
		printf("[^_^] Pass test-lab3-part2-3-basic\n");
	}

	return 0;
}

