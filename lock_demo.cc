//
// Lock demo
//

#include "lock_protocol.h"
#include "lock_client.h"
#include "lock_client_cache.h"
#include "rpc.h"
#include <arpa/inet.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

std::string dst;
lock_client *lc;

int
main(int argc, char *argv[]) {
    int r;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s [host:]port\n type", argv[0]);
        exit(1);
    }

    dst = argv[1];
    lc = new lock_client(dst);
    int type = atoi(argv[2]);
    if (type == 0) {
        printf("Acquiring shared lock...");
        r = lc->acquire(1, LockKind::Shared);
        sleep(10);
        printf("Releasing shared lock...");
        r = lc->release(1, LockKind::Shared);
    } else {
        printf("Acquiring exclusive lock...");
        r = lc->acquire(1, LockKind::Exclusive);
        sleep(10);
        printf("Releasing exclusive lock...");
        r = lc->release(1, LockKind::Exclusive);
    }

    printf("stat returned %d\n", r);
}