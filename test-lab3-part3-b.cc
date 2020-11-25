// special test2 for OCC: three clients, overlap transactions and there are conflicts

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "ydb_client.h"

using namespace std;


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


int main(int argc, char **argv) {
	if (argc <= 1) {
		printf("Usage: %s <ydb_server_listen_port>\n", argv[0]);
		return 0;
	}

	string r;

	ydb_client y1((string(argv[1])));
	ydb_client y2((string(argv[1])));
	ydb_client y3((string(argv[1])));

	y1.transaction_begin();
	y1.set("a", "100");
	y1.set("b", "200");
	y1.set("c", "300");
	y1.transaction_commit();    // we allow false aborting between concurrency transactions, but non-concurrency transactions should never abort

	y1.transaction_begin();
	y2.transaction_begin();
	y3.transaction_begin();

	r = y1.get("a");
		CHECK(r == "100", "check 1 error");
	y1.set("a", "1100");
	r = y2.get("b");
		CHECK(r == "200", "check 2 error");
	y2.set("b", "2100");
	r = y3.get("c");
		CHECK(r == "300", "check 3 error");
	y3.set("c", "3100");

	r = y1.get("b");
		CHECK(r == "200", "check 4 error");
	y1.set("b", "1200");
	r = y2.get("c");
		CHECK(r == "300", "check 5 error");
	y2.set("c", "2200");
	r = y3.get("a");
		CHECK(r == "100", "check 6 error");
	y3.set("a", "3200");

	y3.transaction_commit();    // we allow false aborting between concurrency transactions, but the first commit of concurrency transactions aborts seems impossible
	CHECK_TRANS_EXCEPTION(y2.transaction_commit(), ydb_protocol::ABORT, "check 7 error");    // here it must abort
	CHECK_TRANS_EXCEPTION(y1.transaction_commit(), ydb_protocol::ABORT, "check 8 error");    // here it must abort


	y2.transaction_begin();
	r = y2.get("a");
		CHECK(r == "3200", "check 9 error");
	r = y2.get("b");
		CHECK(r == "200", "check 10 error");
	r = y2.get("c");
		CHECK(r == "3100", "check 11 error");
	y2.transaction_commit();	

	
	if (global_err_count != 0) {
		printf("[x_x] Fail test-lab3-part3-b\n");
	}
	else {
		printf("[^_^] Pass test-lab3-part3-b\n");
	}

	return 0;
}

