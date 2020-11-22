#include <string>
#include "ydb_protocol.h"
#include "ydb_client.h"

using namespace std;

int main(int argc, char **argv) {
	string r;

	if (argc <= 1) {
		printf("Usage: `%s <ydb_server_listen_port>` or `%s <ydb_server_listen_port>` RESTART\n", argv[0], argv[0]);
		return 0;
	}

	//ydb_client y = ydb_client(string(argv[1]));
	ydb_client y((string(argv[1])));
	
	if (argc == 3 && string(argv[2]) == "RESTART") {
		y.transaction_begin();
		string r1 = y.get("a");
		string r2 = y.get("b");
		string r3 = y.get("0123456789abcdef0123456789ABCDEF");

		//cout << "r1:" << r1 << "_" << "r2:" << r2 << endl;
		y.transaction_commit();

		if (r1 == "50" && r2 == "250" && r3 == "fedcba9876543210FEDCBA9876543210") {
			printf("Equal\n");
		}
		else {
			printf("Diff\n");
		}

		return 0;
	}

	y.transaction_begin();
	y.set("a", "100");
	y.set("b", "200");
	y.set("0123456789abcdef0123456789ABCDEF", "fedcba9876543210FEDCBA9876543210");
	y.transaction_commit();
	
	y.transaction_begin();
	string tmp1 = y.get("a");
	string tmp2 = y.get("b");
	int t1 = stoi(tmp1);
	int t2 = stoi(tmp2);
	y.set("a", to_string(t1-50));
	y.set("b", to_string(t2+50));
	y.transaction_commit();
	
	/*
	y.transaction_begin();
	string r1 = y.get("a");
	string r2 = y.get("b");
	//cout << "r1: " << r1 << endl << "r2: " << r2 << endl;
	y.transaction_commit();
	*/
	
	return 0;
}

