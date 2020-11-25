#include "rpc.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "ydb_server.h"
#include "ydb_server_2pl.h"
#include "ydb_server_occ.h"

// Main loop of ydb server

int main(int argc, char *argv[]) {
	int count = 0;

	if(argc != 5){
		fprintf(stderr, "Usage: %s <transaction-type> <port-listen> <port-extent_server> <port-lock_server>\n", argv[0]);
		exit(1);
	}
	std::string transaction_type(argv[1]);
	char *port_listen = argv[2];
	char *port_ec = argv[3];
	char *port_lc = argv[4];
	
	setvbuf(stdout, NULL, _IONBF, 0);

	char *count_env = getenv("RPC_COUNT");
	if(count_env != NULL){
		count = atoi(count_env);
	}

	rpcs server(atoi(port_listen), count);

	if (transaction_type == "2PL") {
		ydb_server_2pl ls(port_ec, port_lc);
		server.reg(ydb_protocol::transaction_begin, &ls, &ydb_server_2pl::transaction_begin);
		server.reg(ydb_protocol::transaction_commit, &ls, &ydb_server_2pl::transaction_commit);
		server.reg(ydb_protocol::transaction_abort, &ls, &ydb_server_2pl::transaction_abort);
		server.reg(ydb_protocol::get, &ls, &ydb_server_2pl::get);
		server.reg(ydb_protocol::set, &ls, &ydb_server_2pl::set);
		server.reg(ydb_protocol::del, &ls, &ydb_server_2pl::del);

		while(1) {    // important! after the if, ls will be deconstructed !
			sleep(1000);
		}
	}
	else if (transaction_type == "OCC") {
		ydb_server_occ ls(port_ec, port_lc);
		server.reg(ydb_protocol::transaction_begin, &ls, &ydb_server_occ::transaction_begin);
		server.reg(ydb_protocol::transaction_commit, &ls, &ydb_server_occ::transaction_commit);
		server.reg(ydb_protocol::transaction_abort, &ls, &ydb_server_occ::transaction_abort);
		server.reg(ydb_protocol::get, &ls, &ydb_server_occ::get);
		server.reg(ydb_protocol::set, &ls, &ydb_server_occ::set);
		server.reg(ydb_protocol::del, &ls, &ydb_server_occ::del);

		while(1) {
			sleep(1000);
		}
	}
	else if (transaction_type == "NONE") {    // no transaction support
		ydb_server ls(port_ec, port_lc);
		server.reg(ydb_protocol::transaction_begin, &ls, &ydb_server::transaction_begin);
		server.reg(ydb_protocol::transaction_commit, &ls, &ydb_server::transaction_commit);
		server.reg(ydb_protocol::transaction_abort, &ls, &ydb_server::transaction_abort);
		server.reg(ydb_protocol::get, &ls, &ydb_server::get);
		server.reg(ydb_protocol::set, &ls, &ydb_server::set);
		server.reg(ydb_protocol::del, &ls, &ydb_server::del);

		while(1) {
			sleep(1000);
		}
	}
	else {
		fprintf(stderr, "Error: <transaction-type> must be '2PL' 'OCC' or 'NONE'\n");
		exit(1);
	}
}

