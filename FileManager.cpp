#include "FileManager.h"

#include <string>

int FileManager::alloc() { // Find lowest 0
    int wh = 0;
    __uint128_t flag_tmp = flag;
    for (int i = MAX_FILE_NUM_INDEX - 1; i >= 0; i--) {
        if ((PREFIX_128(1 << i) & flag_tmp) == PREFIX_128(1 << i)) {
            wh += 1 << i;
            flag_tmp >>= 1 << i;
        }
    }
    if (flag_tmp & 1) return -1; // all bits is 1
    return wh;
}

void FileManager::setFlag(int wh, bool val) {
    if (val) {
        flag |= (__uint128_t)1 << wh; 
    } else {
        flag -= (__uint128_t)1 << wh;
    }
}

FileManager::FileManager() {
    flag = 0;
}

FileManager::~FileManager() {
    for (int i = 0; i < MAX_FILE_NUM; i++) {
        if (flag & ((__uint128_t)1 << i)) closeFile(i);
    }
}

bool FileManager::createFile(const char* name) {
    FILE* f = fopen(name, "a+");
    if (f == NULL) return false;
    fclose(f);
    return true;
}

bool FileManager::openFile(const char* name, int& fileID) {
    int f = open(name, O_RDWR);
    if (f == -1) return false;
    fileID = alloc();
    setFlag(fileID, 1);
    fd[fileID] = f;
    return true;
}

bool FileManager::closeFile(int fileID) {
    setFlag(fileID, 0);
    close(fd[fileID]);
    return true;
}

int FileManager::readPage(int fileID, int pageID, BufType buf) {
    int f = fd[fileID];
    off_t offset = pageID << PAGE_SIZE_INDEX;
    off_t error = lseek(f, offset, SEEK_SET);
    if (error != offset) return -1;
    read(f, buf, PAGE_SIZE);
    return 0;
}

int FileManager::writePage(int fileID, int pageID, BufType buf) {
    int f = fd[fileID];
    off_t offset = pageID << PAGE_SIZE_INDEX;
    off_t error = lseek(f, offset, SEEK_SET);
    if (error != offset) return -1;
    write(f, buf, PAGE_SIZE);
    return 0;
}

char* ReadFile(const char* path)
{
    int length;
	FILE* file = fopen(path, "rb");
	if (file == nullptr) {
		return nullptr;
	}
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	char* data = (char *)malloc((length + 1) * sizeof(char));
	rewind(file);
	length = fread(data, 1, length, file);
	data[length] = '\0';
	fclose(file);
	return data;
}

void WriteFile(const char* path, const char* data, const int length)
{
	FILE* file = fopen(path, "wb");
    fwrite(data, 1, length, file);
	fclose(file);
}

char* Dir(const char* dir, const char* path) {
    int length = strlen(dir) + 1 + strlen(path);
    char* data = (char *)malloc((length + 1) * sizeof(char));
    strcpy(data, dir);
    strcpy(data + strlen(dir), "/");
    strcpy(data + strlen(dir) + 1, path);
    return data;
}

char* Dir(const char* dir, const char* filename, const char* suffix) {
    int length = strlen(dir) + 1 + strlen(filename) + strlen(suffix);
    char* data = (char *)malloc((length + 1) * sizeof(char));
    strcpy(data, dir);
    strcpy(data + strlen(dir), "/");
    strcpy(data + strlen(dir) + 1, filename);
    strcpy(data + strlen(dir) + 1 + strlen(filename), suffix);
    return data;
}

char* Dir(const char* dir, const char* filename, const char* name, const char* suffix) {
    int length = strlen(dir) + 1 + strlen(filename) + 1 + strlen(name) + strlen(suffix);
    char* data = (char *)malloc((length + 1) * sizeof(char));
    strcpy(data, dir);
    strcpy(data + strlen(dir), "/");
    strcpy(data + strlen(dir) + 1, filename);
    strcpy(data + strlen(dir) + 1 + strlen(filename), "_");
    strcpy(data + strlen(dir) + 1 + strlen(filename) + 1, name);
    strcpy(data + strlen(dir) + 1 + strlen(filename) + 1 + strlen(name), suffix);
    return data;
}
