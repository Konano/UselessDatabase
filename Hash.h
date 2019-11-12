#ifndef __HASH
#define __HASH

#include "constants.h"

#include <string.h>

class BufPageManager;

class Hash {
private:
    uint16_t table[MAX_BUF_NUM << 2];
    bool flag[MAX_BUF_NUM << 2];
    BufPageManager* bpm;
    inline int hash(int fileID, int pageID);

public:

    Hash(BufPageManager* bpm) : bpm(bpm) {
        memset(flag, 0, sizeof(flag));
    }
    ~Hash() {}
    int find(int fileID, int pageID);
    void add(int fileID, int pageID, int index);
    void del(int fileID, int pageID);
};

#endif