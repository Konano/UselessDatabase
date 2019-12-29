#include "Hash.h"

#include "BufPageManager.h"

inline int Hash::hash(int fileID, int pageID) {
    return ((pageID   % (MAX_BUF_NUM << 2)) * HASH_PRIME  + fileID)  % (MAX_BUF_NUM << 2);
}

int Hash::find(int fileID, int pageID) {
    int id = hash(fileID, pageID);
    while (flag[id] && bpm->check(fileID, pageID, table[id]) == false) {
        id++;
    }
    if (flag[id] == false) return -1;
    return table[id];
}

void Hash::add(int fileID, int pageID, int index) {
    int id = hash(fileID, pageID);
    while (flag[id]) id++;
    flag[id] = true;
    table[id] = (uint16_t)index;
}

void Hash::del(int fileID, int pageID) {
    int id = hash(fileID, pageID);
    while (flag[id] && bpm->check(fileID, pageID, table[id]) == false) {
        id++;
    }
    if (flag[id]) flag[id] = false, table[id] = 0;
}