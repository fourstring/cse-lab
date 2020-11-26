#!/bin/bash
#set -x

if ! [[ "$1" == "2PL" || "$1" == "OCC" || "$1" == "NONE" ]]; then
        echo "Usage: $0 [2PL/OCC/NONE]"
        exit 1
fi

timeout 30s ./test-lab3-durability 4772

pkill -9 ydb_server
(./ydb_server $1 4772 2772 3772 >ydb_server.log &)
sleep 2    # enlarge the waiting time

rm tmp.tt 2>/dev/null
timeout 30s ./test-lab3-durability 4772 RESTART >tmp.tt

cat tmp.tt 2>/dev/null | grep -q 'Equal'
if [ $? -ne 0 ]; then
	echo "[x_x] Fail test-lab3-durability"
else
	echo "[^_^] Pass test-lab3-durability"
fi
rm tmp.tt

