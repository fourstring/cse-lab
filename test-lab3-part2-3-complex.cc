#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "ydb_client.h"

//#define DEBUG

using namespace std;

static std::string int_to_string(int i) {
	char buf[16];
	sprintf(buf, "%d", i);
	return std::string(buf);
}

/*
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
*/

#define RETRY_UNTIL_OK(instructions) do { \
	bool retry = false; \
	do { \
		retry = false; \
		try { \
			do { \
				instructions \
			} while(0); \
		} \
		catch(ydb_protocol::status e) { \
			(void)e; \
			retry = true; \
		} \
	} while(retry); \
} while(0)



int global_err_count = 0;


#define MAX_PRODUCT_KIND 5

#define USER_COUNT 10
#define TRADE_PER_USER 100

#define PRICE_OF_PRODUCT(i) (i+1)
#define INITIAL_USER_MONEY 1000
#define INITIAL_PRODUCT_COUNT 1000

struct user_struct {    // in database
	int money;
	short buyed_product_count_list[MAX_PRODUCT_KIND];
};

struct product_struct {    // in database
	int price;
	int count;
};


struct record_struct {    // locally
	int user;
	short to_buy_product_count_list[MAX_PRODUCT_KIND];
};


record_struct global_trades_of_user[USER_COUNT][TRADE_PER_USER];
int global_successful_trades_list_size[USER_COUNT];
record_struct global_successful_trades_list[USER_COUNT][TRADE_PER_USER];


bool do_a_buy(ydb_client &y, int user, short *to_buy_product_count_list) {
	bool success = true;
	RETRY_UNTIL_OK(
		do {
			success = true;
			user_struct u;
			y.transaction_begin();
			string user_buf = y.get("user_"+int_to_string(user));
			memcpy(&u, user_buf.c_str(), sizeof(user_struct));
			for(int i = 0; i < MAX_PRODUCT_KIND; i++) {
				int to_buy_count = to_buy_product_count_list[i];
				if (to_buy_count != 0) {
					product_struct p;
					string product_buf = y.get("product_"+int_to_string(i));
					memcpy(&p, product_buf.c_str(), sizeof(product_struct));    // check p.size() ?
					if (!(p.count >= to_buy_count && u.money >= p.price*to_buy_count)) {
						y.transaction_abort();
						success = false;
						break;
					}
					p.count -= to_buy_count;
					product_buf.assign(reinterpret_cast<char *>(&p), sizeof(product_struct));
					y.set("product_"+int_to_string(i), product_buf);    // just set, because we can abort the transaction to recovery the origin value
					u.money -= p.price*to_buy_count;
					u.buyed_product_count_list[i] += to_buy_count;
				}
			}
			if (!success) {
				break;
			}
			user_buf.assign(reinterpret_cast<char *>(&u), sizeof(user_struct));
			y.set("user_"+int_to_string(user), user_buf);
			y.transaction_commit();
			success = true;
		} while(0);
	);
	return success;
}


void init_database(ydb_client &y) {
	y.transaction_begin();
	for(int i = 0; i < MAX_PRODUCT_KIND; i++) {
		product_struct p;
		memset(&p, 0, sizeof(product_struct));
		p.price = PRICE_OF_PRODUCT(i);    // 5 products, price is 1, 2, 3, 4, 5
		p.count = INITIAL_PRODUCT_COUNT;    // initial count is 1000
		string product_buf(reinterpret_cast<char *>(&p), sizeof(product_struct));
		y.set("product_"+int_to_string(i), product_buf);
	}
	for(int i = 0; i < USER_COUNT; i++) {
		user_struct u;
		memset(&u, 0, sizeof(user_struct));
		u.money = INITIAL_USER_MONEY;    // each user has 1000 money
		string user_buf(reinterpret_cast<char *>(&u), sizeof(user_struct));
		y.set("user_"+int_to_string(i), user_buf);
	}
	y.transaction_commit();
}

void generate_trades(ydb_client &y) {
	srand(2020);
	for(int user = 0; user < USER_COUNT; user++) {
		for(int j = 0; j < TRADE_PER_USER; j++) {
			for(int k = 0; k < 3; k++) {
				short product = rand() % MAX_PRODUCT_KIND;
				short to_buy_count = (rand() % 3) + 1;
				global_trades_of_user[user][j].user = user;
				global_trades_of_user[user][j].to_buy_product_count_list[product] = to_buy_count;
			}
		}
	}
}


struct start_buy_arg_struct {
	ydb_client *y;
	int user;
	record_struct (*trades)[TRADE_PER_USER];
};

void *start_buy(void *arg_) {
	start_buy_arg_struct *arg = reinterpret_cast<start_buy_arg_struct *>(arg_);
	for(int i = 0; i < TRADE_PER_USER; i++) {
		bool r = do_a_buy(*arg->y, arg->user, (*arg->trades)[i].to_buy_product_count_list);
		if (r) {
			int index = global_successful_trades_list_size[arg->user]++;
			global_successful_trades_list[arg->user][index] = (*arg->trades)[i];
		}
	}
	return NULL;
}

bool check_final_state(ydb_client &y) {
	user_struct logged_user_final_state[USER_COUNT];
	product_struct logged_product_final_state[MAX_PRODUCT_KIND];

	for(int i = 0; i < USER_COUNT; i++) {
		memset(&logged_user_final_state[i], 0, sizeof(user_struct));
		logged_user_final_state[i].money = INITIAL_USER_MONEY;
	}
	for(int i = 0; i < MAX_PRODUCT_KIND; i++) {
		memset(&logged_product_final_state[i], 0, sizeof(product_struct));
		logged_product_final_state[i].count = INITIAL_PRODUCT_COUNT;
	}

	for(int user = 0; user < USER_COUNT; user++) {
		user_struct *u = &logged_user_final_state[user];
#ifdef DEBUG
		printf("user%d successfully finishes %d trades\n", user, global_successful_trades_list_size[user]);
#endif
		for(int index = 0; index < global_successful_trades_list_size[user]; index++) {
			record_struct *tmp = &global_successful_trades_list[user][index];
			for(int product = 0; product < MAX_PRODUCT_KIND; product++) {
				product_struct *p = &logged_product_final_state[product];
				int price = PRICE_OF_PRODUCT(product);
				int to_buy_count = tmp->to_buy_product_count_list[product];
				logged_user_final_state[user].buyed_product_count_list[product] += to_buy_count;
				u->money -= price * to_buy_count;
				p->count -= to_buy_count;
			}
		}
	}


	user_struct database_user_final_state[USER_COUNT];
	product_struct database_product_final_state[MAX_PRODUCT_KIND];

	y.transaction_begin();
	for(int user = 0; user < USER_COUNT; user++) {
		user_struct *u = &database_user_final_state[user];
		string user_buf = y.get("user_"+int_to_string(user));
		memcpy(u, user_buf.c_str(), sizeof(user_struct));
	}
	for(int product = 0; product < MAX_PRODUCT_KIND; product++) {
		product_struct *p = &database_product_final_state[product];
		string product_buf = y.get("product_"+int_to_string(product));
		memcpy(p, product_buf.c_str(), sizeof(product_struct));
	
	}
	y.transaction_commit();


	bool success = true;
	for(int user = 0; user < USER_COUNT; user++) {
		user_struct *cu = &logged_user_final_state[user];
		user_struct *du = &database_user_final_state[user];
#ifdef DEBUG
		printf("user%d money should be %d, actual is %d\n", user, cu->money, du->money);
#endif
		if (cu->money < 0 || cu->money > 20) {
			printf("error: user money error\n");
			success = false;
		}
		for(int product = 0; product < MAX_PRODUCT_KIND; product++) {
			int correct_count = cu->buyed_product_count_list[product];
			int actual_count = du->buyed_product_count_list[product];
#ifdef DEBUG
			printf("\tuser%d should have %d product%d, actual is %d\n", user, correct_count, product, actual_count);
#endif
			if (correct_count != actual_count) {
				printf("\terror: user buyed product count error\n");
				success = false;
			}
		}
	}
	for(int product = 0; product < MAX_PRODUCT_KIND; product++) {
		product_struct *cp = &logged_product_final_state[product];
		product_struct *dp = &database_product_final_state[product];
#ifdef DEBUG
		printf("product%d count should be %d, actual is %d\n", product, cp->count, dp->count);
#endif
		if (cp->count < 0 || cp->count != dp->count) {
#ifdef DEBUG
			printf("error: product count error\n");
#endif
			success = false;
		}
	}

	return success;
}


pthread_t per_user_thread_id[USER_COUNT];
start_buy_arg_struct global_start_buy_arg_list[USER_COUNT];

int main(int argc, char **argv) {
	if (argc <= 1) {
		printf("Usage: %s <ydb_server_listen_port>\n", argv[0]);
		return 0;
	}

	string ydb_server_port(argv[1]);
	ydb_client y(ydb_server_port);

	init_database(y);
	generate_trades(y);

	for(int user = 0; user < USER_COUNT; user++) {
		global_start_buy_arg_list[user].user = user;
		global_start_buy_arg_list[user].y = new ydb_client(ydb_server_port);
		global_start_buy_arg_list[user].trades = &global_trades_of_user[user];
		pthread_create(&per_user_thread_id[user], NULL, start_buy, &global_start_buy_arg_list[user]);
	}
	for(int user = 0; user < USER_COUNT; user++) {
		pthread_join(per_user_thread_id[user], NULL);
		delete global_start_buy_arg_list[user].y;
	}

	bool success = check_final_state(y);
	if (success) {
		printf("[^_^] Pass test-lab3-part2-3-complex\n");
	}
	else {
		printf("[x_x] Fail test-lab3-part2-3-complex\n");
	}

	return 0;
}

