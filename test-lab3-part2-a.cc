// special test1 for 2PL: three threads, but each thread does not have transaction conflict

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

void *start_thread0(void *arg_) {
	(void)arg_;

	string r;
	ydb_client y((global_ydb_server_port));

	y.transaction_begin();
	y.set("thread0_a", "100");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 3);
	y.set("thread0_b", "200");
	y.transaction_commit();
	
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 6);

	y.transaction_begin();
	r = y.get("thread0_a");
		CHECK(r == "100", "thread0 check 1 error");
	y.set("thread0_a", "101");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 9);
	r = y.get("thread0_b");
		CHECK(r == "200", "thread0 check 2 error");
	y.set("thread0_b", "201");
	y.transaction_commit();
	
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 12);

	y.transaction_begin();
	r = y.get("thread0_a");
		CHECK(r == "101", "thread0 check 3 error");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 15);
	r = y.get("thread0_b");
		CHECK(r == "201", "thread0 check 4 error");
	y.transaction_commit();
	
	return NULL;
}

void *start_thread1(void *arg_) {
	(void)arg_;
	
	string r;
	ydb_client y((global_ydb_server_port));

	y.transaction_begin();
	y.set("thread1_a", "1100");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 3);
	y.set("thread1_b", "1200");
	y.transaction_commit();
	
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 6);

	y.transaction_begin();
	r = y.get("thread1_a");
		CHECK(r == "1100", "thread1 check 1 error");
	y.set("thread1_a", "1101");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 9);
	r = y.get("thread1_b");
		CHECK(r == "1200", "thread1 check 2 error");
	y.set("thread1_b", "1201");
	y.transaction_commit();
	
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 12);
	
	y.transaction_begin();
	r = y.get("thread1_a");
		CHECK(r == "1101", "thread1 check 3 error");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 15);
	r = y.get("thread1_b");
		CHECK(r == "1201", "thread1 check 4 error");
	y.transaction_commit();
	
	return NULL;
}

void *start_thread2(void *arg_) {
	(void)arg_;
	
	string r;
	ydb_client y((global_ydb_server_port));

	y.transaction_begin();
	y.set("thread2_a", "2100");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 3);
	y.set("thread2_b", "2200");
	y.transaction_commit();
	
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 6);

	y.transaction_begin();
	r = y.get("thread2_a");
		CHECK(r == "2100", "thread2 check 1 error");
	y.set("thread2_a", "2101");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 9);
	r = y.get("thread2_b");
		CHECK(r == "2200", "thread2 check 2 error");
	y.set("thread2_b", "2201");
	y.transaction_commit();
	
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 12);
	
	y.transaction_begin();
	r = y.get("thread2_a");
		CHECK(r == "2101", "thread2 check 3 error");
	__sync_fetch_and_add(&global_counter, 1); while(global_counter < 15);
	r = y.get("thread2_b");
		CHECK(r == "2201", "thread2 check 4 error");
	y.transaction_commit();
	
	return NULL;
}


int main(int argc, char **argv) {
	if (argc <= 1) {
		printf("Usage: %s <ydb_server_listen_port>\n", argv[0]);
		return 0;
	}

	global_ydb_server_port = string(argv[1]);

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
		printf("[x_x] Fail test-lab3-part2-a\n");
	}
	else {
		printf("[^_^] Pass test-lab3-part2-a\n");
	}

	return 0;
}

