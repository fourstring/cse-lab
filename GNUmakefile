LAB=3
SOL=0
RPC=./rpc
LAB1GE=$(shell expr $(LAB) \>\= 1)
LAB2GE=$(shell expr $(LAB) \>\= 2)
LAB3GE=$(shell expr $(LAB) \>\= 3)
LAB4GE=$(shell expr $(LAB) \>\= 4)
#CXXFLAGS =  -g -MMD -Wall -I. -I$(RPC) -DLAB=$(LAB) -DSOL=$(SOL) -D_FILE_OFFSET_BITS=64 -no-pie -D_GLIBCXX_USE_CXX11_ABI=0
CXXFLAGS = -std=c++17 -g -MMD -Wall -I. -I$(RPC) -DLAB=$(LAB) -DSOL=$(SOL) -D_FILE_OFFSET_BITS=64
FUSEFLAGS= -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=25 -I/usr/local/include/fuse -I/usr/include/fuse

# choose librpc based on architecture
ifeq ($(shell getconf LONG_BIT),64)
	RPCLIB= librpc64.a
else
	RPCLIB= librpc86.a
endif

ifeq ($(shell uname -s),Darwin)
  MACFLAGS= -D__FreeBSD__=10
else
  MACFLAGS=
endif
#LDFLAGS = -L. -L/usr/local/lib -no-pie
LDFLAGS = -L. -L/usr/local/lib
LDLIBS = -lpthread 
ifeq ($(LAB1GE),1)
  ifeq ($(shell uname -s),Darwin)
    ifeq ($(shell sw_vers -productVersion | sed -e "s/.*\(10\.[0-9]\).*/\1/"),10.6)
      LDLIBS += -lfuse_ino64
    else
      LDLIBS += -lfuse
    endif
  else
    LDLIBS += -lfuse
  endif
endif
LDLIBS += $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS += $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
CC = g++
CXX = g++

lab:  lab$(LAB)
lab1: part1_tester yfs_client
lab2: lock_server lock_tester lock_demo yfs_client extent_server test-lab2-part1-g test-lab2-part2-a test-lab2-part2-b test-lab2-part3-a test-lab2-part3-b
lab3: lock_server extent_server ydb_server test-lab3-durability test-lab3-part2-3-basic test-lab3-part2-a test-lab3-part2-b test-lab3-part3-a test-lab3-part3-b test-lab3-part2-3-complex  yfs_client test-lab2-part1-g test-lab2-part2-a test-lab2-part2-b test-lab2-part3-a test-lab2-part3-b

hfiles1=rpc/fifo.h rpc/connection.h rpc/rpc.h rpc/marshall.h rpc/method_thread.h\
	rpc/thr_pool.h rpc/pollmgr.h rpc/jsl_log.h rpc/slock.h rpc/rpctest.cc\
	lock_protocol.h lock_server.h lock_client.h gettime.h gettime.cc lang/verify.h \
        lang/algorithm.h
hfiles2=yfs_client.h extent_client.h extent_protocol.h extent_server.h lock_client_cache.h lock_server_cache.h handle.h tprintf.h
hfiles3=lock_client_cache.h lock_server_cache.h handle.h tprintf.h

rpclib=rpc/rpc.cc rpc/connection.cc rpc/pollmgr.cc rpc/thr_pool.cc rpc/jsl_log.cc gettime.cc
rpc/$(RPCLIB): $(patsubst %.cc,%.o,$(rpclib))
	rm -f $@
	ar cq $@ $^
	ranlib $@

rpc/rpctest=rpc/rpctest.cc
rpc/rpctest: $(patsubst %.cc,%.o,$(rpctest)) rpc/$(RPCLIB)

lock_demo=lock_demo.cc lock_client.cc
lock_demo : $(patsubst %.cc,%.o,$(lock_demo)) rpc/$(RPCLIB)

lock_tester=lock_tester.cc lock_client.cc lock_client_cache.cc
lock_tester : $(patsubst %.cc,%.o,$(lock_tester)) rpc/$(RPCLIB)

lock_server=lock_server.cc lock_smain.cc lock_server_cache.cc handle.cc
lock_server : $(patsubst %.cc,%.o,$(lock_server)) rpc/$(RPCLIB)

part1_tester=part1_tester.cc extent_client.cc extent_server.cc Directory.cc inode_manager.cc
part1_tester : $(patsubst %.cc,%.o,$(part1_tester))
yfs_client=yfs_client.cc extent_client.cc fuse.cc extent_server.cc Directory.cc inode_manager.cc
ifeq ($(LAB2GE),1)
  yfs_client += lock_client.cc lock_client_cache.cc
endif
yfs_client : $(patsubst %.cc,%.o,$(yfs_client)) rpc/$(RPCLIB)

extent_server=extent_server.cc extent_smain.cc Directory.cc inode_manager.cc
extent_server : $(patsubst %.cc,%.o,$(extent_server)) rpc/$(RPCLIB)

ydb_server=ydb_server.cc ydb_server_2pl.cc ydb_server_occ.cc ydb_smain.cc extent_client.cc lock_client.cc lock_client_cache.cc
ydb_server : $(patsubst %.cc,%.o,$(ydb_server)) rpc/$(RPCLIB)

#ydb_client=ydb_client.cc ydb
#ydb_client : $(patsubst %.cc,%.o,$(ydb_client)) rpc/$(RPCLIB)

test-lab2-part1-b=test-lab2-part1-b.c
test-lab2-part1-b:  $(patsubst %.c,%.o,$(test-lab2-part1-b)) rpc/$(RPCLIB)

test-lab2-part1-c=test-lab2-part1-c.c
test-lab2-part2-c:  $(patsubst %.c,%.o,$(test-lab2-part1-c)) rpc/$(RPCLIB)


test-lab3-durability=test-lab3-durability.cc ydb_client.cc
test-lab3-durability: $(patsubst %.cc,%.o,$(test-lab3-durability)) rpc/$(RPCLIB)

test-lab3-part2-3-basic=test-lab3-part2-3-basic.cc ydb_client.cc
test-lab3-part2-3-basic: $(patsubst %.cc,%.o,$(test-lab3-part2-3-basic)) rpc/$(RPCLIB)

test-lab3-part2-a=test-lab3-part2-a.cc ydb_client.cc
test-lab3-part2-a: $(patsubst %.cc,%.o,$(test-lab3-part2-a)) rpc/$(RPCLIB)

test-lab3-part2-b=test-lab3-part2-b.cc ydb_client.cc
test-lab3-part2-b: $(patsubst %.cc,%.o,$(test-lab3-part2-b)) rpc/$(RPCLIB)

test-lab3-part3-a=test-lab3-part3-a.cc ydb_client.cc
test-lab3-part3-a: $(patsubst %.cc,%.o,$(test-lab3-part3-a)) rpc/$(RPCLIB)

test-lab3-part3-b=test-lab3-part3-b.cc ydb_client.cc
test-lab3-part3-b: $(patsubst %.cc,%.o,$(test-lab3-part3-b)) rpc/$(RPCLIB)

test-lab3-part2-3-complex=test-lab3-part2-3-complex.cc ydb_client.cc
test-lab3-part2-3-complex: $(patsubst %.cc,%.o,$(test-lab3-part2-3-complex)) rpc/$(RPCLIB)


%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

fuse.o: fuse.cc
	$(CXX) -c $(CXXFLAGS) $(FUSEFLAGS) $(MACFLAGS) $<

# mklab.inc is needed by 6.824 staff only. Just ignore it.
-include mklab.inc

-include *.d
-include rpc/*.d

clean_files=rpc/*.a rpc/rpctest rpc/*.o rpc/*.d *.o *.d yfs_client extent_server lock_server lock_tester lock_demo rpctest ydb_server test-lab2-part1-a test-lab2-part1-b test-lab2-part1-c test-lab2-part1-g test-lab2-part2-a test-lab2-part2-b test-lab2-part3-a test-lab2-part3-b part1_tester demo_client demo_server test-lab3-durability test-lab3-part2-3-basic test-lab3-part2-a test-lab3-part2-b test-lab3-part3-a test-lab3-part3-b test-lab3-part2-3-complex
.PHONY: clean handin
clean: 
	rm $(clean_files) -rf 

handin_ignore=$(clean_files) core* *log
handin_file=lab$(LAB).tgz
labdir=$(shell basename $(PWD))
handin: 
	@bash -c "cd ../; tar -X <(tr ' ' '\n' < <(echo '$(handin_ignore)')) -czvf $(handin_file) $(labdir); mv $(handin_file) $(labdir); cd $(labdir)"
	@echo Please modify lab3.tgz to lab3_[your student id].tgz and upload it to ftp://huangzheng:public@public.sjtu.edu.cn/upload/cse/lab3/
	@echo Thanks!

rpcdemo: demo_server demo_client

demo_client:
	$(CXX) $(CXXFLAGS) demo_client.cc rpc/$(RPCLIB) $(LDFLAGS) $(LDLIBS) -o demo_client

demo_server:
	$(CXX) $(CXXFLAGS) demo_server.cc rpc/$(RPCLIB) $(LDFLAGS) $(LDLIBS) -o demo_server
