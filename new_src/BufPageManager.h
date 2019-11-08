#ifndef __BUF_PAGE_MANAGER
#define __BUF_PAGE_MANAGER

#include "FileManager.h"
#include <string.h>

#define MAX_BUF_NUM 65536
#define HASH_PRIME 131

typedef uint8_t* BufType;

class BufPageManager;

class Hash {
private:
    uint16_t table[MAX_BUF_NUM << 2];
    bool flag[MAX_BUF_NUM << 2];
    BufPageManager* bpm;

    inline int hash(int fileID, int pageID) {
        return pageID * HASH_PRIME + fileID;
    }

public:

    Hash(BufPageManager* bpm) : bpm(bpm) {
        memset(flag, 0, sizeof(flag));
    }

    ~Hash() {
        delete bpm;
    }

    int find(int fileID, int pageID) {
        int id = hash(fileID, pageID) % (MAX_BUF_NUM << 2);
        while (flag[id] && bpm->check(fileID, pageID, table[id]) == false) {
            id++;
        }
        if (flag[id] == false) return -1;
        return table[id];
    }

    void add(int fileID, int pageID, int index) {
        int id = hash(fileID, pageID) % (MAX_BUF_NUM << 2);
        while (flag[id]) id++;
        flag[id] = true;
        table[id] = (uint16_t)index;
    }

    void del(int fileID, int pageID) {
        int id = hash(fileID, pageID) % (MAX_BUF_NUM << 2);
        while (flag[id] && bpm->check(fileID, pageID, table[id]) == false) {
            id++;
        }
        if (flag[id]) flag[id] = table[id] = 0;
    }
};

class MemPool {
private:

    struct node {
        int pre, next;
    };

    node pool[MAX_BUF_NUM];
    int num, head, tail;

    inline void link(int a, int b) {
        pool[a].next = b;
        pool[b].pre = a;
    }

    // inline void dislink(int a, int b) {
    //     pool[a].next = pool[b].pre = 0;
    // }

public:
    MemPool() {
        num = 0;
        head = tail = -1;
    }

    void access(int index) {

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

    int alloc() {
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
};

class BufPageManager {
private:

    FileManager* fm;
    BufType addr[MAX_BUF_NUM];
    int file_id[MAX_BUF_NUM];
    int page_id[MAX_BUF_NUM];
    bool dirty[MAX_BUF_NUM];
    Hash* hash;
    MemPool* pool;

    BufType fetchPage(int fileID, int pageID, int& index) {
        index = pool->alloc();

        if (addr[index] != nullptr) {
            writeBack(index);
        } else {
            addr[index] = new uint8_t[PAGE_SIZE];
        }
        file_id[index] = fileID;
        page_id[index] = pageID;
        hash->add(fileID, pageID, index);
        
        return addr[index];
    }

    // writeBack will throw the buf away
    void writeBack(int index) {
        if (dirty[index]) {
            fm->writePage(file_id[index], page_id[index], addr[index]);
            dirty[index] = false;
        }
        // pool->free(index);
        hash->del(file_id[index], page_id[index]);
        // file_id[index] = page_id[index] = 0;
        // addr[index] = nullptr;
    }

public:

    BufPageManager(FileManager* fm) : fm(fm) {
        hash = new Hash(this);
        pool = new MemPool();
    }

    ~BufPageManager() {
        for (int i = 0; i < MAX_BUF_NUM; ++ i) {
			writeBack(i);
            delete addr[i];
		}
        delete fm;
        delete hash;
        delete pool;
    }

    BufType getPage(int fileID, int pageID, int& index) {
		index = hash->find(fileID, pageID);
		if (index != -1) {
			pool->access(index);
			return addr[index];
		} else {
			BufType buf = fetchPage(fileID, pageID, index);
			fm->readPage(fileID, pageID, buf);
			return buf;
		}
	}

    void markDirty(int index) {
        dirty[index] = true;
        pool->access(index);
    }

    bool check(int fileID, int pageID, int index) {
        return file_id[index] == fileID && page_id[index] == pageID;
    }
};

#endif