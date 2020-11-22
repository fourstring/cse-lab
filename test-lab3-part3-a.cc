// special test1 for OCC: three clients, overlap transactions but no conflict

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


/*
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
*/


int global_err_count = 0;


int main(int argc, char **argv) {
	if (argc <= 1) {
		printf("Usage: %s <ydb_server_listen_port>\n", argv[0]);
		return 0;
	}

	string r;
	ydb_client y1((std::string(argv[1])));
	ydb_client y2((std::string(argv[1])));
	ydb_client y3((std::string(argv[1])));

	y1.transaction_begin();
	y1.set("a1", "1100");
	y2.transaction_begin();
	y2.set("a2", "2100");
	y3.transaction_begin();
	y3.set("a3", "3100");
	y1.set("b1", "1200");
	y1.transaction_commit();
	y2.set("b2", "2200");
	y3.set("b3", "3200");
	y3.transaction_commit();
	y2.transaction_commit();


	y1.transaction_begin();
	r = y1.get("a1");
		CHECK(r == "1100", "check 1 error");
	y2.transaction_begin();
	r = y2.get("a2");
		CHECK(r == "2100", "check 2 error");
	y1.set("a1", "1101");
	y2.set("a2", "2101");
	r = y1.get("b1");
		CHECK(r == "1200", "check 3 error");
	y3.transaction_begin();
	y1.set("b1", "1201");
	r = y3.get("a3");
		CHECK(r == "3100", "check 4 error");
	r = y2.get("b2");
		CHECK(r == "2200", "check 5 error");
	y1.transaction_commit();
	y3.set("a3", "3101");
	y2.set("b2", "2201");
	y2.transaction_commit();
	r = y3.get("b3");
		CHECK(r == "3200", "check 6 error");
	y3.set("b3", "3201");
	y3.transaction_commit();


	y1.transaction_begin();
	r = y1.get("a1");
		CHECK(r == "1101", "check 7 error");
	y3.transaction_begin();
	r = y3.get("a3");
		CHECK(r == "3101", "check 8 error");
	y2.transaction_begin();
	r = y2.get("a2");
		CHECK(r == "2101", "check 9 error");
	r = y2.get("b2");
		CHECK(r == "2201", "check 10 error");
	r = y3.get("b3");
		CHECK(r == "3201", "check 11 error");
	y3.transaction_commit();
	r = y1.get("b1");
		CHECK(r == "1201", "check 12 error");
	y2.transaction_commit();
	y1.transaction_commit();



	if (global_err_count != 0) {
		printf("[x_x] Fail test-lab3-part3-a\n");
	}
	else {
		printf("[^_^] Pass test-lab3-part3-a\n");
	}

	return 0;
}

