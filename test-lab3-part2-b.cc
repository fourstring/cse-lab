// special test2 for 2PL: three threads, but the locks are cyclic dependent so it will cause dead lock, ydb_server should solve the problem by abort one transaction.

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include "ydb_client.h"

using namespace std;


#define CHECK(express, errmsg) do { \
	if (!(express)) { \
		printf("Error%d : '%s' at %s:%d : %s \n", __sync_fetch_and_add(&global_err_count, 1), #express, __FILE__, __LINE__, errmsg); \
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

volatile int global_counter = 0;

string global_ydb_server_port;

bool t0_aborted = false;
bool t1_aborted = false;
bool t2_aborted = false;

void *start_thread0(void *arg_) {
	(void)arg_;

	string r;
	ydb_client y((global_ydb_server_port));

	y.transaction_begin();
	y.set("a", "0100");

	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 3);

	try {
		y.set("b", "0200");
		y.transaction_commit();
	}
	catch(ydb_protocol::status e) {
		CHECK(e == ydb_protocol::ABORT, "check t0 abort error");
		t0_aborted = true;
	}

	return NULL;
}

void *start_thread1(void *arg_) {
	(void)arg_;
	
	string r;
	ydb_client y((global_ydb_server_port));

	y.transaction_begin();
	y.set("b", "1200");
	
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 3);
	
	try {
		y.set("c", "1300");
		y.transaction_commit();
	}
	catch(ydb_protocol::status e) {
		CHECK(e == ydb_protocol::ABORT, "check t1 abort error");
		t1_aborted = true;
	}

	return NULL;
}

void *start_thread2(void *arg_) {
	(void)arg_;
	
	string r;
	ydb_client y((global_ydb_server_port));

	y.transaction_begin();
	y.set("c", "2300");

	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 3);
	
	try {
		y.set("a", "2100");
		y.transaction_commit();
	}
	catch(ydb_protocol::status e) {
		CHECK(e == ydb_protocol::ABORT, "check t2 abort error");
		t2_aborted = true;
	}

	return NULL;
}


int main(int argc, char **argv) {
	if (argc <= 1) {
		printf("Usage: %s <ydb_server_listen_port>\n", argv[0]);
		return 0;
	}

	global_ydb_server_port = string(argv[1]);

	ydb_client y((global_ydb_server_port));

	y.transaction_begin();
	y.set("a", "100");
	y.set("b", "200");
	y.set("c", "300");
	y.transaction_commit();

	pthread_t t0;
	pthread_t t1;
	pthread_t t2;

	pthread_create(&t0, NULL, start_thread0, NULL);
	pthread_create(&t1, NULL, start_thread1, NULL);
	pthread_create(&t2, NULL, start_thread2, NULL);

	pthread_join(t0, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	y.transaction_begin();
	string r1 = y.get("a");
	string r2 = y.get("b");
	string r3 = y.get("c");
	y.transaction_commit();

	// we should not assume which thread to abort (the old version of this test assumes the last transaction to abort, which is wrong)
	// but, we can know there is one and should only one transaction aborted
	
	//printf("%d %d %d\n", t0_aborted, t1_aborted, t2_aborted);
	
	if (t0_aborted && !t1_aborted && !t2_aborted) {    // only transaction 0 aborted
		CHECK(r1 == "2100", "t0 abort check a error");
		CHECK(r2 == "1200", "t0 abort check b error");
		CHECK(r3 == "1300", "t0 abort check c error");
	}
	else if (!t0_aborted && t1_aborted && !t2_aborted) {    // only transaction 1 aborted
		CHECK(r1 == "2100", "t1 abort check a error");
		CHECK(r2 == "0200", "t1 abort check b error");
		CHECK(r3 == "2300", "t1 abort check c error");
	}
	else if (!t0_aborted && !t1_aborted && t2_aborted) {    // only transaction 2 aborted
		CHECK(r1 == "0100", "t2 abort check a error");
		CHECK(r2 == "0200", "t2 abort check b error");
		CHECK(r3 == "1300", "t2 abort check c error");
	}
	else {
		CHECK(false, "check only one transaction abort error");
	}

	if (global_err_count != 0) {
		printf("[x_x] Fail test-lab3-part2-b\n");
	}
	else {
		printf("[^_^] Pass test-lab3-part2-b\n");
	}

	return 0;
}

