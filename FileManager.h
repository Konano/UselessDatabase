#ifndef __FILE_MANAGER
#define __FILE_MANAGER

#include "constants.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

class BufPageManager;

#define PREFIX_128(x) (((__uint128_t)1 << (x)) - 1)

class FileManager {
private:
    int fd[MAX_FILE_NUM];
    __uint128_t flag;
    BufPageManager* bpm;

    int alloc();
    void setFlag(int wh, bool val);

public:
    FileManager();
    ~FileManager();
    bool createFile(const char* name);
    bool openFile(const char* name, int& fileID);
    bool closeFile(int fileID);
    int readPage(int fileID, int pageID, BufType buf);
    int writePage(int fileID, int pageID, BufType buf);
    void setBPM(BufPageManager* bpm);
};

#endif