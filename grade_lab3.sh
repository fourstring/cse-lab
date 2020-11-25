#!/bin/bash
#set -x

passedtest=0


test_none(){
	echo "Start testing part1 : no transaction"
	echo ""

	echo "start no transaction test-lab3-durability"
	./start_ydb.sh NONE
	rm tmp.t 2>/dev/null
	./test-lab3-durability.sh NONE | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part1"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part1"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""

	echo "Finish testing part1 : no transaction"
}


test_2pl(){
	echo "Start testing part2 : 2PL"
	echo ""

	echo "start 2PL test-lab3-durability"
	./start_ydb.sh 2PL
	rm tmp.t 2>/dev/null
	./test-lab3-durability.sh 2PL | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part2 durability"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part2 durability"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""
	
	echo "start 2PL test-lab3-part2-3-basic"
	./start_ydb.sh 2PL
	rm tmp.t 2>/dev/null
	timeout 30s ./test-lab3-part2-3-basic 4772 | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part2 basic"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part2 basic"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""

	echo "start 2PL test-lab3-part2-a"
	./start_ydb.sh 2PL
	rm tmp.t 2>/dev/null
	timeout 30s ./test-lab3-part2-a 4772 | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part2 special a"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part2 special a"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""

	echo "start 2PL test-lab3-part2-b"
	./start_ydb.sh 2PL
	rm tmp.t 2>/dev/null
	timeout 30s ./test-lab3-part2-b 4772 | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part2 special b"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part2 special b"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""

	echo "start 2PL test-lab3-part2-3-complex"
	./start_ydb.sh 2PL
	rm tmp.t 2>/dev/null
	timeout 300s ./test-lab3-part2-3-complex 4772 | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part2 complex"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part2 complex"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""

	echo "Finish testing part2 : 2PL"
}


test_occ(){
	echo "Start testing part3 : OCC"
	echo ""
	
	echo "start OCC test-lab3-durability"
	./start_ydb.sh OCC
	rm tmp.t 2>/dev/null
	./test-lab3-durability.sh OCC | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part3 durability"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part3 durability"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""
	
	echo "start OCC test-lab3-part2-3-basic"
	./start_ydb.sh OCC
	rm tmp.t 2>/dev/null
	timeout 30s ./test-lab3-part2-3-basic 4772 | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part3 basic"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part3 basic"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""

	echo "start OCC test-lab3-part3-a"
	./start_ydb.sh OCC
	rm tmp.t 2>/dev/null
	timeout 30s ./test-lab3-part3-a 4772 | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part3 special a"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part3 special a"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""

	echo "start OCC test-lab3-part3-b"
	./start_ydb.sh OCC
	rm tmp.t 2>/dev/null
	timeout 30s ./test-lab3-part3-b 4772 | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part3 special b"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part3 special b"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""

	echo "start OCC test-lab3-part2-3-complex"
	./start_ydb.sh OCC
	rm tmp.t 2>/dev/null
	timeout 300s ./test-lab3-part2-3-complex 4772 | tee tmp.t
	cat tmp.t 2>/dev/null | grep -q '[^_^] Pass'
	if [ $? -ne 0 ]; then
        	echo "Failed part3 complex"
	else
		passedtest=$((passedtest+1))
        	echo "Passed part3 complex"
	fi
	rm tmp.t 2>/dev/null
	./stop_ydb.sh
	echo ""

	echo "Finish testing part3 : OCC"
}

test_none ; echo "" ; echo "----------------------------------------" ; echo ""
test_2pl ; echo "" ; echo "----------------------------------------" ; echo ""
test_occ ; echo "" ; echo "----------------------------------------" ; echo ""

echo "Your passed $passedtest/11 tests."
echo ""

