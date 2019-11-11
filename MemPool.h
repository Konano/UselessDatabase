#ifndef __MEMPOOL
#define __MEMPOOL

#include "constants.h"

class MemPool {
private:
    struct node {
        int pre, next;
    };
    node pool[MAX_BUF_NUM];
    int num, head, tail;

    inline void link(int a, int b);

public:
    MemPool();
    void access(int index);
    int alloc();
};

#endif