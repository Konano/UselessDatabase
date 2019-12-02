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
extern void replaceFile(const char *oldName, const char *newName);

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
    j["pri_key"] = pri_key;
    return j;
}

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
    pri_key = j["pri_key"];
}

uint Sheet::calDataSize() {
    uint size = 0;
    for(uint i = 0; i < col_num; i++) {
        size += col_ty[i].size();
    }
    return size;
}

int Sheet::createSheet(Database* db, const char* name, int col_num, Type* col_ty, bool create) {
    for (int i = 0; i < col_num; i++) {
        if (col_ty[i].key == enumKeyType::Primary) {
            // TODO: when add primary, should ...
            // need unique, non-null, no-def
        }
        if (col_ty[i].key == enumKeyType::Foreign) {
            if (db->sheet[col_ty[i].foreign_sheet]->pri_key == -1) return -1;
        }
    }
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
    return 0;
}

int Sheet::insertRecord(Any* info) {
    int index;
    BufType buf = bpm->getPage(main_file, record_num / record_onepg, index);
    buf[(record_num % record_onepg) / 8] |= 1 << (record_num % 8);
    buf += (record_onepg - 1) / 8 + 1;
    buf += record_size * (record_num % record_onepg);
    for(uint i = 0; i < col_num; i++) {
        if (info[i].anyCast<int>() != NULL) {
            *(uint32_t*)buf = *info[i].anyCast<int>();
            buf += 4;
        }
        if (info[i].anyCast<char*>() != NULL) {
            memcpy(buf, *info[i].anyCast<char*>(), col_ty[i].size());
            buf += col_ty[i].size();
        }
        if (col_ty[i].key == enumKeyType::Primary) {
            // TODO: when add primary, should ...
            // need unique, non-null, no-def
        }
        if (col_ty[i].key == enumKeyType::Foreign) {
            if (!info[i].isNull() && !db->sheet[col_ty[i].foreign_sheet]->queryPrimaryKey(info[i])) {
                return -1;
            }
        }
    }
    for (uint i = 0; i < index_num; i++) {
        this->index[i].insertRecord(&info[this->index[i].key], record_num);
    }
    bpm->markDirty(index);
    record_num++;
    return 0;
}

int Sheet::removeRecord(const int record_id) {
    int index;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
    if (buf[(record_id % record_onepg) / 8] & (1 << (record_id % 8))) {
        Any* info;
        queryRecord(record_id, info);
        for (uint i = 0; i < index_num; i++) {
            this->index[i].removeRecord(&info[this->index[i].key], record_id);
        }
        buf[(record_id % record_onepg) / 8] -= 1 << (record_id % 8);
        bpm->markDirty(index);
        return 0;
    }
    return -1;
}

int Sheet::queryRecord(const int record_id, Any* &info) {
    int index;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
    if (buf[(record_id % record_onepg) / 8] & (1 << (record_id % 8))) {
        buf += (record_onepg - 1) / 8 + 1;
        buf += record_size * (record_id % record_onepg);
        info = (Any*) malloc(col_num * sizeof(Any));
        memset(info, 0, col_num * sizeof(Any));
        for(uint i = 0; i < col_num; i++) {
            if (this->col_ty[i].ty == enumType::INT) {
                info[i] = *(uint32_t*)buf;
                buf += 4;
            }
            if (this->col_ty[i].ty == enumType::CHAR) {
                char* str = (char *)malloc((col_ty[i].size()+1) * sizeof(char));
                memcpy(str, buf, col_ty[i].size());
                str[col_ty[i].size()] = '\0';
                info[i] = str;
                buf += col_ty[i].size();
            }
        }
        return 0;
    }
    return -1;
}

int Sheet::updateRecord(const int record_id, const int len, Any* info) {
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
            memcpy(buf, *info[i].anyCast<char*>(), col_ty[i].size());
            buf += col_ty[i].size();
        }
        if (col_ty[i].key == enumKeyType::Primary) {
            // TODO: when add primary, should ...
            // need unique, non-null, no-def
        }
        if (col_ty[i].key == enumKeyType::Foreign) {
            if (!info[i].isNull() && !db->sheet[col_ty[i].foreign_sheet]->queryPrimaryKey(info[i])) {
                return -1;
            }
        }
    }
    bpm->markDirty(index);
    return 0;
}

bool Sheet::queryPrimaryKey(Any query_val) {
    if (pri_key == -1) return false;
    int index;
    BufType buf, buf_st;
    for (uint i = 0; i < record_num; i++) {
        if (i % record_onepg == 0) {
            buf_st = buf = bpm->getPage(main_file, i / record_onepg, index);
            buf += (record_onepg - 1) / 8 + 1;
        }
        if (buf_st[(i % record_onepg) / 8] & (1 << (i % 8))) {
            if (this->col_ty[pri_key].ty == enumType::INT) {
                if (Any(*(uint32_t*)buf) == query_val) return true;
                buf += 4;
            }
            if (this->col_ty[pri_key].ty == enumType::CHAR) {
                char* str = (char *)malloc((col_ty[pri_key].size()+1) * sizeof(char));
                memcpy(str, buf, col_ty[pri_key].size());
                str[col_ty[pri_key].size()] = '\0';
                if (Any(str) == query_val) return true;
                buf += col_ty[pri_key].size();
            }
        }
    }
    return false;
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

inline char* getTime() {
    time_t rawtime;
    time( &rawtime );
    char* buffer = (char *)malloc(MAX_NAME_LEN * sizeof(char));
    strftime(buffer, MAX_NAME_LEN, "%Y%m%d%H%M%S", localtime( &rawtime ));
    return buffer;
}

void Sheet::createIndex(uint key_index) {
    index[index_num] = Index(this, getTime(), key_index, 3);
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
                    char* str = (char *)malloc((col_ty[i].size()+1) * sizeof(char));
                    memcpy(str, _buf, col_ty[i].size());
                    str[col_ty[i].size()] = '\0';
                    info[i] = str;
                    _buf += col_ty[i].size();
                }
            }
            index[index_num].insertRecord(&info[index[index_num].key], record_id);
        }
    }
    index_num++;
}

void Sheet::removeIndex(uint index_id) {
    index[index_id].close();
    index[index_id].remove();
    index[index_id] = index[--index_num];
}

void Sheet::createColumn(Type ty) {
    col_ty[col_num++] = ty;
    rebuild(1, col_num);
}

void Sheet::removeColumn(uint key_index) {
    rebuild(0, key_index);
    for (uint i = key_index; i < col_num - 1; i++) {
        col_ty[i] = col_ty[i + 1];
    }
    col_num -= 1;
}

void Sheet::modifyColumn(uint key_index, Type ty) {
    col_ty[key_index] = ty;
}

void Sheet::rebuild(int ty, uint key_index) {
    int _main_file;
    fm->createFile(dirPath(db->name, name, "_tmp.usid"));
    fm->openFile(dirPath(db->name, name, "_tmp.usid"), _main_file);
    uint _record_size, _record_onepg;
    int index, _index;
    BufType buf, _buf, buf_st, _buf_st;
    switch (ty) {
    case 0: // del
        _record_size = record_size - col_ty[key_index].size();
        _record_onepg = 0;
        while ((_record_onepg + 1) * _record_size + _record_onepg / 8 + 1 <= PAGE_SIZE) _record_onepg++;
        for (uint i = 0; i < record_num; i++) {
            if (i % record_onepg == 0) {
                buf_st = buf = bpm->getPage(main_file, i / record_onepg, index);
                buf += (record_onepg - 1) / 8 + 1;
            }
            if (i % _record_onepg == 0) {
                _buf_st = _buf = bpm->getPage(_main_file, i / _record_onepg, _index);
                _buf += (_record_onepg - 1) / 8 + 1;
            }
            if (buf_st[(i % record_onepg) / 8] & (1 << (i % 8))) 
                _buf_st[(i % _record_onepg) / 8] |= 1 << (i % 8);
            else if (_buf_st[(i % record_onepg) / 8] & (1 << (i % 8)))
                _buf_st[(i % _record_onepg) / 8] -= 1 << (i % 8);
            for(uint j = 0; j < col_num; j++) {
                if (col_ty[j].ty == enumType::INT) {
                    if (j != key_index) *(uint32_t*)_buf = *(uint32_t*)buf;
                    buf += 4; 
                    if (j != key_index) _buf += 4; 
                }
                if (col_ty[j].ty == enumType::CHAR) {
                    if (j != key_index) memcpy(_buf, buf, col_ty[j].size());
                    buf += col_ty[j].size();
                    if (j != key_index) _buf += col_ty[j].size();
                }
            }
            bpm->markDirty(_index);
        }
        break;
    case 1: // add
        _record_size = record_size + col_ty[key_index].size();
        _record_onepg = 0;
        while ((_record_onepg + 1) * _record_size + _record_onepg / 8 + 1 <= PAGE_SIZE) _record_onepg++;
        for (uint i = 0; i < record_num; i++) {
            if (i % record_onepg == 0) {
                buf_st = buf = bpm->getPage(main_file, i / record_onepg, index);
                buf += (record_onepg - 1) / 8 + 1;
            }
            if (i % _record_onepg == 0) {
                _buf_st = _buf = bpm->getPage(_main_file, i / _record_onepg, _index);
                _buf += (_record_onepg - 1) / 8 + 1;
            }
            if (buf_st[(i % record_onepg) / 8] & (1 << (i % 8))) 
                _buf_st[(i % _record_onepg) / 8] |= 1 << (i % 8);
            else if (_buf_st[(i % record_onepg) / 8] & (1 << (i % 8)))
                _buf_st[(i % _record_onepg) / 8] -= 1 << (i % 8);
            for(uint j = 0; j < col_num - 1; j++) {
                if (col_ty[j].ty == enumType::INT) {
                    *(uint32_t*)_buf = *(uint32_t*)buf;
                    buf += 4; _buf += 4; 
                }
                if (col_ty[j].ty == enumType::CHAR) {
                    memcpy(_buf, buf, col_ty[j].size());
                    buf += col_ty[j].size(); _buf += col_ty[j].size();
                }
            }
            Any info = col_ty[col_num - 1].def;
            if (info.anyCast<int>() != NULL) {
                *(uint32_t*)_buf = *info.anyCast<int>();
                _buf += 4;
            }
            if (info.anyCast<char*>() != NULL) {
                memcpy(_buf, *info.anyCast<char*>(), col_ty[col_num - 1].size());
                _buf += col_ty[col_num - 1].size();
            }
            bpm->markDirty(_index);
        }
        break;
    }
    fm->closeFile(main_file);
    fm->closeFile(_main_file);
    replaceFile(dirPath(db->name, name, "_tmp.usid"), dirPath(db->name, name, ".usid"));

    fm->openFile(dirPath(db->name, name, ".usid"), main_file);
    record_size = _record_size;
    record_onepg = _record_onepg;
}

int Sheet::createForeignKey(uint key_index, uint sheet_id) {
    if (db->sheet[sheet_id]->pri_key == -1) return -1;
    return col_ty[key_index].setForeignKey(sheet_id);
}

int Sheet::removeForeignKey(uint key_index) {
    return col_ty[key_index].unsetForeignKey();
}