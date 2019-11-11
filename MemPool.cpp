#include "MemPool.h"

inline void MemPool::link(int a, int b) {
    pool[a].next = b;
    pool[b].pre = a;
}

// inline void MemPool::dislink(int a, int b) {
//     pool[a].next = pool[b].pre = 0;
// }

MemPool::MemPool() {
    num = 0;
    head = tail = -1;
}

void MemPool::access(int index) {

    if (index == head) {
        return;
    } else if (index == tail) {
        tail = pool[index].pre;
    } else {
        link(pool[index].pre, pool[index].next);
    }
    link(index, head);
    head = index;
}

int MemPool::alloc() {
    if (num == 0) {
        head = tail = num;
        return num++;
    } 
    if (num < MAX_BUF_NUM) {
        link(num, head);
        head = num;
        return num++;
    }
    access(tail);
    return head;
}