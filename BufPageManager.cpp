#include "BufPageManager.h"

#include "FileManager.h"
#include "Hash.h"
#include "MemPool.h"

BufType BufPageManager::fetchPage(int fileID, int pageID, int& index) {
    index = pool->alloc();

    if (addr[index] != nullptr) {
        writeBack(index);
    } else {
        addr[index] = new uint8_t[PAGE_SIZE];
        memset(addr[index], 0, PAGE_SIZE);
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
    hash->del(file_id[index], page_id[index]);
}

BufPageManager::BufPageManager(FileManager* fm) : fm(fm) {
    fm->setBPM(this);
    hash = new Hash(this);
    pool = new MemPool();
    lastFileID = lastPageID = lastIndex = -1;
    memset(addr, 0, sizeof(addr));
    memset(file_id, 0, sizeof(file_id));
    memset(page_id, 0, sizeof(page_id));
    memset(dirty, 0, sizeof(dirty));
}

BufPageManager::~BufPageManager() {
    for (int i = 0; i < MAX_BUF_NUM; ++ i) {
        writeBack(i);
        delete addr[i];
    }
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

void BufPageManager::closeFile(int fileID) {
    for (int i = 0; i < MAX_BUF_NUM; ++ i) {
        if (file_id[i] == fileID) {
            writeBack(i);
            delete addr[i];
            addr[i] = nullptr;
        }
    }
}