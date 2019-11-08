#ifndef __FILE_MANAGER
#define __FILE_MANAGER

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#define MAX_FILE_NUM 128
#define MAX_FILE_NUM_INDEX 7

#define PAGE_SIZE 8192
#define PAGE_SIZE_INDEX 13

typedef uint8_t* BufType;

#define PREFIX_128(x) (((__uint128_t)1 << (x)) - 1)

class FileManager {
private:
    int fd[MAX_FILE_NUM];
    __uint128_t flag;

    int alloc() { // Find lowest 0
        int wh = 0;
        __uint128_t flag_tmp = flag;
        for (int i = MAX_FILE_NUM_INDEX - 1; i >= 0; i++) {
            if ((PREFIX_128(1 << i) & flag_tmp) == PREFIX_128(1 << i)) {
                wh += 1 << i;
                flag_tmp >>= 1 << i;
            }
        }
        if (flag_tmp & 1) return -1; // all bits is 1
        return wh;
    }

    void setFlag(int wh, bool val) {
        if (val) {
            flag |= (__uint128_t)1 << wh; 
        } else {
            flag -= (__uint128_t)1 << wh;
        }
    }

public:
    FileManager() {
        flag = 0;
    }

    ~FileManager() {
        for (int i = 0; i < MAX_FILE_NUM; i++) {
            if (flag & ((__uint128_t)1 << i)) closeFile(i);
        }
    }

    bool createFile(const char* name) {
		FILE* f = fopen(name, "a+");
		if (f == NULL) return false;
		fclose(f);
		return true;
	}

    bool openFile(const char* name, int& fileID) {
        int f = open(name, O_RDWR);
        if (f == -1) return false;
		fileID = alloc();
		setFlag(fileID, 1);
        fd[fileID] = f;
		return true;
	}

    bool closeFile(int fileID) {
        setFlag(fileID, 0);
        close(fd[fileID]);
        return true;
    }

    int readPage(int fileID, int pageID, BufType buf) {
        int f = fd[fileID];
        off_t offset = pageID << PAGE_SIZE_INDEX;
        off_t error = lseek(f, offset, SEEK_SET);
        if (error != offset) return -1;
        read(f, buf, PAGE_SIZE);
        return 0;
    }

    int writePage(int fileID, int pageID, BufType buf) {
        int f = fd[fileID];
        off_t offset = pageID << PAGE_SIZE_INDEX;
        off_t error = lseek(f, offset, SEEK_SET);
        if (error != offset) return -1;
        write(f, buf, PAGE_SIZE);
        return 0;
    }
};

#endif