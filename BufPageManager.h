#ifndef __BUF_PAGE_MANAGER
#define __BUF_PAGE_MANAGER

#include "constants.h"

#include <string.h>

class Hash;
class MemPool;
class FileManager;

class BufPageManager {
private:
    FileManager* fm;
    BufType addr[MAX_BUF_NUM];
    int file_id[MAX_BUF_NUM];
    int page_id[MAX_BUF_NUM];
    bool dirty[MAX_BUF_NUM];
    Hash* hash;
    MemPool* pool;
    
    int lastFileID, lastPageID, lastIndex;
    BufType lastBuf;

    BufType fetchPage(int fileID, int pageID, int& index);
    // writeBack will throw the buf away
    void writeBack(int index);
    
public:
    BufPageManager(FileManager* fm);
    ~BufPageManager();
    BufType getPage(int fileID, int pageID, int& index);
    void markDirty(int index);
    bool check(int fileID, int pageID, int index);
    void closeFile(int fileID);
};

#endif