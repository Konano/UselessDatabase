#include "Sheet.h"

#include "Database.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include "Type.h"
#include "Index.h"

#include <time.h>
#include <iostream>
using namespace std;

extern char* dirPath(const char* dir, const char* filename, const char* suffix);

// class Sheet {
// public:
//     char name[MAX_NAME_LEN];
//     // char comment[MAX_COMMENT_LEN];
//     Database* db;
//     FileManager* fm;
//     BufPageManager* bpm;
//     uint col_num = 0;
//     Type col_ty[MAX_COL_NUM];
//     // ForeignKey f_key[MAX_COL_NUM];
//     // int index_num = 0;
//     // Index index[MAX_INDEX_NUM];
//     uint record_num = 0;
//     int main_file;
//     uint record_size;
//     uint record_onepg;

//     // char* getFileName(int ty);
//     uint calDataSize();
//     Sheet(Database* db, const char* name, int col_num, Type* col_ty);
//     void insertRecord(const int len, Any* info);
//     void removeRecord(const int record_id);
//     int quertRecord(const int record_id);

//     json toJson();
//     void fromJson(json j);
// };

json Sheet::toJson() {
    json j;
    j["name"] = name;
    j["col_num"] = col_num;
    for (uint i = 0; i < col_num; i++) {
        j["col_ty"].push_back(col_ty[i].toJson());
    }
    j["record_num"] = record_num;
    j["record_size"] = record_size;
    j["record_onepg"] = record_onepg;
    j["index_num"] = index_num;
    for (uint i = 0; i < index_num; i++) {
        j["index"].push_back(index[i].toJson());
    }
    return j;
}

// char* Sheet::getFileName(int ty) {
//     if (ty == -2) {
//         // TODO: varchar
//     } else if (ty == -1) {
//         char *filename = new char[MAX_NAME_LEN + 6];
//         strcpy(filename, name);
//         strcpy(filename + strlen(name), ".usid\0");
//         return filename;
//     } else {
//         // TODO: index
//     }
//     return nullptr;
// }

uint Sheet::calDataSize() {
    uint size = 0;
    for(uint i = 0; i < col_num; i++) {
        size += col_ty[i].size();
    }
    return size;
}

Sheet::Sheet(Database* db, const char* name, int col_num, Type* col_ty, bool create) {
    this->db = db;
    this->fm = db->fm;
    this->bpm = db->bpm;
    strcpy(this->name, name);
    this->col_num = col_num;
    memcpy(this->col_ty, col_ty, sizeof(Type) * col_num);
    if (create) {
        fm->createFile(dirPath(db->name, name, ".usid"));
    }
    fm->openFile(dirPath(db->name, name, ".usid"), main_file);
    record_size = calDataSize();
    record_onepg = 0;
    while ((record_onepg + 1) * record_size + record_onepg / 8 + 1 <= PAGE_SIZE) record_onepg++;
}

void Sheet::insertRecord(const int len, Any* info) {
    int index;
    BufType buf = bpm->getPage(main_file, record_num / record_onepg, index);
    buf[(record_num % record_onepg) / 8] |= 1 << (record_num % 8);
    buf += (record_onepg - 1) / 8 + 1;
    buf += record_size * (record_num % record_onepg);
    for(int i = 0; i < len; i++) {
        if (info[i].anyCast<int>() != NULL) {
            *(uint32_t*)buf = *info[i].anyCast<int>();
            buf += 4;
        }
        if (info[i].anyCast<char*>() != NULL) {
            memcpy(buf, *info[i].anyCast<char*>(), col_ty[i].len);
            buf += col_ty[i].len;
        }
    }
    for (uint i = 0; i < index_num; i++) {
        this->index[i].insertRecord(len, info, record_num);
    }
    bpm->markDirty(index);
    record_num++;
}

int Sheet::removeRecord(const int record_id) {
    int index;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
    if (buf[(record_id % record_onepg) / 8] & (1 << (record_id % 8))) {
        buf[(record_id % record_onepg) / 8] -= 1 << (record_id % 8);
        bpm->markDirty(index);
        return 0;
    }
    return -1;
}

int Sheet::queryRecord(const int record_id, const int len, Any* &info) {
    int index;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
    if (buf[(record_id % record_onepg) / 8] & (1 << (record_id % 8))) {
        buf += (record_onepg - 1) / 8 + 1;
        buf += record_size * (record_id % record_onepg);
        info = (Any*) malloc(len * sizeof(Any));
        memset(info, 0, len * sizeof(Any));
        for(int i = 0; i < len; i++) {
            if (this->col_ty[i].ty == enumType::INT) {
                info[i] = *(int*)buf;
                buf += 4;
            }
            if (this->col_ty[i].ty == enumType::CHAR) {
                char* str = (char *)malloc((col_ty[i].len+1) * sizeof(char));
                memcpy(str, buf, col_ty[i].len);
                str[col_ty[i].len] = '\0';
                info[i] = str;
                buf += col_ty[i].len;
            }
        }
        return 0;
    }
    return -1;
}

void Sheet::updateRecord(const int record_id, const int len, Any* info) {
    int index;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
    buf += (record_onepg - 1) / 8 + 1;
    buf += record_size * (record_id % record_onepg);
    for(int i = 0; i < len; i++) {
        if (info[i].anyCast<int>() != NULL) {
            *(uint32_t*)buf = *info[i].anyCast<int>();
            buf += 4;
        }
        if (info[i].anyCast<char*>() != NULL) {
            memcpy(buf, *info[i].anyCast<char*>(), col_ty[i].len);
            buf += col_ty[i].len;
        }
    }
    bpm->markDirty(index);
}

// json Sheet::toJson() {
//     json j;
//     j["name"] = name;
//     j["col_num"] = col_num;
//     for (uint i = 0; i < col_num; i++) {
//         j["col_ty"].push_back(col_ty[i].toJson());
//     }
//     j["record_num"] = record_num;
//     j["record_size"] = record_size;
//     j["record_onepg"] = record_onepg;
//     return j;
// }

Sheet::Sheet(Database* db, json j) { // from JSON
    this->db = db;
    this->fm = db->fm;
    this->bpm = db->bpm;
    strcpy(name, j["name"].get<std::string>().c_str());
    col_num = j["col_num"].get<int>();
    record_num = j["record_num"].get<int>();
    record_size = j["record_size"].get<int>();
    record_onepg = j["record_onepg"].get<int>();
    for (uint i = 0; i < col_num; i++) {
        col_ty[i] = Type(j["col_ty"][i]);
    }
    fm->openFile(dirPath(db->name, name, ".usid"), main_file);
    index_num = j["index_num"].get<int>();
    for (uint i = 0; i < index_num; i++) {
        index[i] = Index(this, j["index"][i]);
    }
}

inline char* getTime() {
    time_t rawtime;
    time( &rawtime );
    char* buffer = (char *)malloc(MAX_NAME_LEN * sizeof(char));
    strftime(buffer, MAX_NAME_LEN, "%Y%m%d%H%M%S", localtime( &rawtime ));
    return buffer;
}

void Sheet::createIndex(uint key_index) {
    index[index_num] = Index(this, getTime(), key_index, 3, 0);
    index[index_num].open();

    int _index;
    for (uint record_id = 0; record_id < record_num; record_id++) {
        BufType buf = bpm->getPage(main_file, record_id / record_onepg, _index);
        if (buf[(record_id % record_onepg) / 8] & (1 << (record_id % 8))) {

            BufType _buf = buf + (record_onepg - 1) / 8 + 1 + record_size * (record_id % record_onepg);
            Any* info = (Any*) malloc(col_num * sizeof(Any));
            memset(info, 0, col_num * sizeof(Any));
            for(uint i = 0; i < col_num; i++) {
                if (this->col_ty[i].ty == enumType::INT) {
                    info[i] = *(int*)_buf;
                    _buf += 4;
                }
                if (this->col_ty[i].ty == enumType::CHAR) {
                    char* str = (char *)malloc((col_ty[i].len+1) * sizeof(char));
                    memcpy(str, _buf, col_ty[i].len);
                    str[col_ty[i].len] = '\0';
                    info[i] = str;
                    _buf += col_ty[i].len;
                }
            }

            index[index_num].insertRecord(col_num, info, record_id);
        }
    }
    index_num++;
}

void Sheet::removeIndex(uint index_id) {
    index[index_id].close();
    index[index_id].remove();
    index[index_id] = index[--index_num];
}

// void Sheet::removeIndex(uint index_id) {

// }