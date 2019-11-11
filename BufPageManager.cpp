#include "BufPageManager.h"

BufType BufPageManager::fetchPage(int fileID, int pageID, int& index) {
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

void BufPageManager::writeBack(int index) {
    if (dirty[index]) {
        fm->writePage(file_id[index], page_id[index], addr[index]);
        dirty[index] = false;
    }
    // pool->free(index);
    hash->del(file_id[index], page_id[index]);
    // file_id[index] = page_id[index] = 0;
    // addr[index] = nullptr;
}

BufPageManager::BufPageManager(FileManager* fm) : fm(fm) {
    hash = new Hash(this);
    pool = new MemPool();
    lastFileID = lastPageID = lastIndex = -1;
}

BufPageManager::~BufPageManager() {
    for (int i = 0; i < MAX_BUF_NUM; ++ i) {
        writeBack(i);
        delete addr[i];
    }
    // delete fm;
    delete hash;
    delete pool;
}

BufType BufPageManager::getPage(int fileID, int pageID, int& index) {
    if (fileID == lastFileID && pageID == lastPageID) {
        index = lastIndex;
        return lastBuf;
    }
    lastFileID = fileID;
    lastPageID = pageID;
    lastIndex = index = hash->find(fileID, pageID);
    if (index != -1) {
        pool->access(index);
        return lastBuf = addr[index];
    } else {
        BufType buf = fetchPage(fileID, pageID, index);
        lastIndex = index;
        fm->readPage(fileID, pageID, buf);
        return lastBuf = buf;
    }
}

void BufPageManager::markDirty(int index) {
    dirty[index] = true;
    pool->access(index);
}

bool BufPageManager::check(int fileID, int pageID, int index) {
    return file_id[index] == fileID && page_id[index] == pageID;
}