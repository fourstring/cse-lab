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

volatile int global_counter = 0;

string global_ydb_server_port;

void *start_thread0(void *arg_) {
	(void)arg_;

	string r;
	ydb_client y((global_ydb_server_port));

	y.transaction_begin();
	y.set("a", "0100");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 3);
	y.set("b", "0200");
	while(global_counter < 4);
	y.transaction_commit();
	
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 7);

	y.transaction_begin();
	r = y.get("a");
		CHECK(r == "0100", "tread0 check 1 error");
	r = y.get("b");
		CHECK(r == "0200", "tread0 check 2 error");
	r = y.get("c");
		CHECK(r == "1300", "tread0 check 3 error");
	y.transaction_commit();
	
	return NULL;
}

void *start_thread1(void *arg_) {
	(void)arg_;
	
	string r;
	ydb_client y((global_ydb_server_port));

	y.transaction_begin();
	y.set("b", "1200");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 3);
	y.set("c", "1300");
	while(global_counter < 4);
	y.transaction_commit();
	
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 7);

	y.transaction_begin();
	r = y.get("a");
		CHECK(r == "0100", "tread1 check 1 error");
	r = y.get("b");
		CHECK(r == "0200", "tread1 check 2 error");
	r = y.get("c");
		CHECK(r == "1300", "tread1 check 3 error");
	y.transaction_commit();

	return NULL;
}

void *start_thread2(void *arg_) {
	(void)arg_;
	
	string r;
	ydb_client y((global_ydb_server_port));

	y.transaction_begin();
	y.set("c", "2300");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 3);
	usleep(1000000/10);    // wait trans0 and trans1 getting in lock
	CHECK_TRANS_EXCEPTION(y.set("a", "2100"), ydb_protocol::ABORT, "thread2 check abort error");
	__sync_fetch_and_add(&global_counter, 1);
	// it is already aborted, so no need to y.transaction_commit();
	// now global_counter should be 4

	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 7);

	y.transaction_begin();
	r = y.get("a");
		CHECK(r == "0100", "tread2 check 1 error");
	r = y.get("b");
		CHECK(r == "0200", "tread2 check 2 error");
	r = y.get("c");
		CHECK(r == "1300", "tread2 check 3 error");
	y.transaction_commit();

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

	if (global_err_count != 0) {
		printf("[x_x] Fail test-lab3-part2-b\n");
	}
	else {
		printf("[^_^] Pass test-lab3-part2-b\n");
	}

	return 0;
}

