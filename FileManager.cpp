#include "FileManager.h"
#include "BufPageManager.h"

#include <string>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

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

inline void FileManager::setFlag(int wh, bool val) {
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

int FileManager::deleteFile(const char* name) {
    return remove(name);
}

bool FileManager::closeFile(int fileID) {
    setFlag(fileID, 0);
    bpm->closeFile(fileID);
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

void FileManager::setBPM(BufPageManager* bpm) {
    this->bpm = bpm;
}

char* readFile(const char* path) {
    int length;
    FILE* file = fopen(path, "rb");
    if (file == nullptr) {
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    char* data = new char[length + 1];
    rewind(file);
    length = fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);
    return data;
}

void writeFile(const char* path, const char* data, const int length) {
    FILE* file = fopen(path, "wb");
    fwrite(data, 1, length, file);
    fclose(file);
}

char* dirPath(const char* dir) { // db / dir
    int length = 3 + strlen(dir);
    char* data = new char[length + 1];
    strcpy(data, "db/");
    strcpy(data + 3, dir);
    return data;
}

char* dirPath(const char* dir, const char* path) { // db / dir / path
    int length = 3 + strlen(dir) + 1 + strlen(path);
    char* data = new char[length + 1];
    strcpy(data, "db/");
    strcpy(data + 3, dir);
    strcpy(data + 3 + strlen(dir), "/");
    strcpy(data + 3 + strlen(dir) + 1, path);
    return data;
}

char* dirPath(const char* dir, const char* filename, const char* filetype) { // db / dir / filename . filetype
    int length = 3 + strlen(dir) + 1 + strlen(filename) + 1 + strlen(filetype);
    char* data = new char[length + 1];
    strcpy(data, "db/");
    strcpy(data + 3, dir);
    strcpy(data + 3 + strlen(dir), "/");
    strcpy(data + 3 + strlen(dir) + 1, filename);
    strcpy(data + 3 + strlen(dir) + 1 + strlen(filename), ".");
    strcpy(data + 3 + strlen(dir) + 1 + strlen(filename) + 1, filetype);
    return data;
}

char* dirPath(const char* dir, const char* filename, const char* subname, const char* filetype) { // db / dir / filename _ subname . filetype
    int length = 3 + strlen(dir) + 1 + strlen(filename) + 1 + strlen(subname) + 1 + strlen(filetype);
    char* data = new char[length + 1];
    strcpy(data, "db/");
    strcpy(data + 3, dir);
    strcpy(data + 3 + strlen(dir), "/");
    strcpy(data + 3 + strlen(dir) + 1, filename);
    strcpy(data + 3 + strlen(dir) + 1 + strlen(filename), "_");
    strcpy(data + 3 + strlen(dir) + 1 + strlen(filename) + 1, subname);
    strcpy(data + 3 + strlen(dir) + 1 + strlen(filename) + 1 + strlen(subname), ".");
    strcpy(data + 3 + strlen(dir) + 1 + strlen(filename) + 1 + strlen(subname) + 1, filetype);
    return data;
}

int cleanFiles(const char *dir) { // clean file(s)
    char dir_name[512];
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    int tmp;

    // if can't access, nothing to do
    if ( 0 != access(dir, F_OK) ) {
        return 0;
    }

    // if can't get direcrtory stat, fail
    if ( 0 > stat(dir, &dir_stat) ) {
        perror("get directory stat error");
        return -1;
    }

    if ( S_ISREG(dir_stat.st_mode) ) { // files
        remove(dir);
    } else if ( S_ISDIR(dir_stat.st_mode) ) { // folder
        dirp = opendir(dir);
        while ( (dp=readdir(dirp)) != NULL ) {
            if ( (0 == strcmp(".", dp->d_name)) || (0 == strcmp("..", dp->d_name)) ) { // Ignore . & ..
                continue;
            }
            sprintf(dir_name, "%s/%s", dir, dp->d_name);
            if ((tmp = cleanFiles(dir_name)) != 0) {
                return tmp;
            }
        }
        closedir(dirp);
        rmdir(dir); // remove empty folder
    } else {
        perror("unknow file type!");
        return -1;    
    }
    
    return 0;
}

void replaceFile(const char *oldName, const char *newName) {
    cleanFiles(newName);
    rename(oldName, newName);
}

int checkFile(const char *filename) { // file exists? 1: exist, 0: null, -1: error
    struct stat dir_stat;
    if ( 0 != access(filename, F_OK) ) return 0;
    if ( 0 > stat(filename, &dir_stat) ) return -1;
    if ( S_ISREG(dir_stat.st_mode) ) return 1;
    return -1;
}