#include "Sheet.h"

#include "Database.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include "Type.h"
#include "Index.h"
#include "Print.h"

#include <time.h>
#include <iostream>
using namespace std;

extern char* dirPath(const char* dir, const char* filename, const char* suffix);
extern void replaceFile(const char *oldName, const char *newName);

json Sheet::toJson() { // assemble to JSON
    json j;
    j["name"] = name;
    j["col_num"] = col_num;
    for (uint i = 0; i < col_num; i++) j["col_ty"].push_back(col_ty[i].toJson());
    j["record_num"] = record_num;
    j["record_size"] = record_size;
    j["record_onepg"] = record_onepg;
    j["index_num"] = index_num;
    for (uint i = 0; i < index_num; i++) j["index"].push_back(index[i].toJson());
    j["pri_key"] = pri_key;
    j["pri_key_index"] = pri_key_index;
    return j;
}

Sheet::Sheet(Database* db, json j) { // disassemble from JSON
    this->db = db;
    this->fm = db->fm;
    this->bpm = db->bpm;
    strcpy(name, j["name"].get<std::string>().c_str());
    col_num = j["col_num"].get<int>();
    record_num = j["record_num"].get<int>();
    record_size = j["record_size"].get<int>();
    record_onepg = j["record_onepg"].get<int>();
    for (uint i = 0; i < col_num; i++) col_ty[i] = Type(j["col_ty"][i]);
    fm->openFile(dirPath(db->name, name, ".usid"), main_file);
    index_num = j["index_num"].get<int>();
    for (uint i = 0; i < index_num; i++) index[i] = Index(this, j["index"][i]);
    pri_key = j["pri_key"];
    pri_key_index = j["pri_key_index"];
}

Sheet::~Sheet() {
    fm->closeFile(main_file);
}

uint Sheet::calDataSize() {
    uint size = 0;
    for(uint i = 0; i < col_num; i++) {
        size += col_ty[i].size();
    }
    return size;
}

int Sheet::createSheet(uint sheet_id, Database* db, const char* name, int col_num, Type* col_ty, bool create) {

    this->sheet_id = sheet_id;
    this->db = db;
    this->fm = db->fm;
    this->bpm = db->bpm;
    strcpy(this->name, name);
    this->col_num = col_num;
    memcpy(this->col_ty, col_ty, sizeof(Type) * col_num);

    bool flag = false;
    for (int i = 0; i < col_num; i++) {
        if (col_ty[i].key == enumKeyType::Primary) {
            this->createIndex(i);
            this->pri_key = i;
            this->pri_key_index = index_num - 1;
            if (flag) return -1;
            else { flag = true; }
        }
        if (col_ty[i].key == enumKeyType::Foreign) {
            if (col_ty[i].foreign_sheet >= db->sheet_num || 
                db->sheet[col_ty[i].foreign_sheet]->pri_key == -1) 
                    return -1;
        }
    }
    if (create) {
        fm->createFile(dirPath(db->name, name, ".usid"));
    }
    fm->openFile(dirPath(db->name, name, ".usid"), main_file);
    record_size = calDataSize();
    record_onepg = 0;
    while ((record_onepg + 1) * record_size + record_onepg / 8 + 1 <= PAGE_SIZE) record_onepg++;
    return 0;
}

char* Sheet::getStr(BufType buf, uint size) {
    char* str = (char *)malloc((size + 1) * sizeof(char));
    memcpy(str, buf, size);
    str[size] = '\0';
    return str;
}

void Sheet::insert(Any& val, enumType ty, uint size, BufType& buf) {
    switch (ty) {
    case INT: 
        *(int*)buf = *val.anyCast<int>(); 
        break;
    case CHAR: 
        memcpy(buf, *val.anyCast<char*>(), size); 
        break;
    case VARCHAR: 
        *(uint64_t*)buf = db->storeVarchar(*val.anyCast<char*>()); 
        break;
    case DATE: 
        *(uint32_t*)buf = *val.anyCast<uint32_t>(); 
        break;
    case DECIMAL: 
        *(long double*)buf = *val.anyCast<long double>(); 
        break;
    }
    buf += size;
}

void Sheet::fetch(BufType& buf, enumType ty, uint size, Any& val) {
    switch (ty) {
    case INT: 
        val = *(int*)buf; 
        break;
    case CHAR:
        val = getStr(buf, size);
        break;
    case VARCHAR: 
        val = db->getVarchar(*(uint64_t*)buf); 
        break;
    case DATE: 
        val = *(int*)buf; 
        break;
    case DECIMAL: 
        val = *(long double*)buf; 
        break;
    }
    buf += size;
}

int Sheet::insertRecord(Any* info) { // TODO VARCHAR 已经被处理了
    int index;
    BufType buf = bpm->getPage(main_file, record_num / record_onepg, index);
    buf[(record_num % record_onepg) / 8] |= 1 << (record_num % 8);
    buf += (record_onepg - 1) / 8 + 1;
    buf += record_size * (record_num % record_onepg);
    for(uint i = 0; i < col_num; i++) {
        if (col_ty[i].isNull() == false && info[i].isNull()) return -1;
        if (col_ty[i].isUnique()) {
            // TODO
        }
        if (col_ty[i].key == enumKeyType::Primary) {
            if (info[i].isNull()) return -1;
            int ans = this->index[pri_key_index].queryRecord(info);
            if (ans != -1) return -1;
        }
        if (col_ty[i].key == enumKeyType::Foreign) {
            if (!info[i].isNull() && !db->sheet[col_ty[i].foreign_sheet]->queryPrimaryKey(info[i])) {
                return -1;
            }
        }
        insert(info[i], col_ty[i].ty, col_ty[i].size(), buf);
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
            fetch(buf, col_ty[i].ty, col_ty[i].size(), info[i]);
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
        if (col_ty[i].key == enumKeyType::Primary) {
            if (info[i].isNull()) return -1;
            int ans = this->index[pri_key_index].queryRecord(info);
            if (ans == -1) return -1;
        }
        if (col_ty[i].key == enumKeyType::Foreign) {
            if (!info[i].isNull() && !db->sheet[col_ty[i].foreign_sheet]->queryPrimaryKey(info[i])) {
                return -1;
            }
        }
        insert(info[i], col_ty[i].ty, col_ty[i].size(), buf);
    }
    bpm->markDirty(index);
    return 0;
}

bool Sheet::queryPrimaryKey(Any query_val) { // TODO 为啥不能去 index 找呢？
    if (pri_key == -1) return false;
    int index;
    BufType buf, buf_st;
    int size_pre = 0;
    for (int i = 0; i < pri_key; i++) 
        size_pre += col_ty[i].size();
    for (uint i = 0; i < record_num; i++) {
        if (i % record_onepg == 0) {
            buf_st = buf = bpm->getPage(main_file, i / record_onepg, index);
            buf += (record_onepg - 1) / 8 + 1;
        }
        if (buf_st[(i % record_onepg) / 8] & (1 << (i % 8))) {
            buf += size_pre;
            switch (this->col_ty[pri_key].ty) {
            case INT:
                if (Any(*(int*)buf) == query_val) return true;
                break;
            case CHAR:
                if (Any(getStr(buf, col_ty[pri_key].size())) == query_val) return true;
                break;
            case VARCHAR:
                if (Any(db->getVarchar(*(uint64_t*)buf)) == query_val) return true;
                break;
            case DATE:
                if (Any(*(int*)buf) == query_val) return true;
                break;
            case DECIMAL:
                if (Any(*(long double*)buf) == query_val) return true;
                break;
            }
            buf -= size_pre;
            buf += record_size;
        }
        buf += record_size;
    }
    return false;
}

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
            Any val;
            fetch(_buf, col_ty[index[index_num].key].ty, col_ty[index[index_num].key].size(), val);
            index[index_num].insertRecord(&val, record_id);
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

void Sheet::rebuild(int ty, uint key_index) { // TODO 需要顺便重构 index
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
                if (j != key_index) memcpy(_buf, buf, col_ty[j].size());
                buf += col_ty[j].size();
                if (j != key_index) _buf += col_ty[j].size();
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
                memcpy(_buf, buf, col_ty[j].size());
                buf += col_ty[j].size(); 
                _buf += col_ty[j].size();
            }
            insert(col_ty[col_num - 1].def, col_ty[col_num - 1].ty, col_ty[col_num - 1].size(), _buf);
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

int Sheet::createPrimaryKey(uint key_index) {
    if (this->pri_key != -1) return -1;

    std::vector<Any> v;

    int _index;
    for (uint record_id = 0; record_id < record_num; record_id++) {
        BufType buf = bpm->getPage(main_file, record_id / record_onepg, _index);
        if (buf[(record_id % record_onepg) / 8] & (1 << (record_id % 8))) {

            BufType _buf = buf + (record_onepg - 1) / 8 + 1 + record_size * (record_id % record_onepg);
            Any* info = (Any*) malloc(col_num * sizeof(Any));
            memset(info, 0, col_num * sizeof(Any));
            for(uint i = 0; i < col_num; i++) {
                fetch(_buf, col_ty[i].ty, col_ty[i].size(), info[i]);
            }
            bool flag = false;
            for(auto a: v){
                if (a == info[key_index])
                {
                    flag = true;
                    break;
                }
            }
            if (!flag) v.push_back(info[key_index]);
            else return -1;
            //index[index_num].insertRecord(&info[index[index_num].key], record_id);
        }
    }

    this->pri_key = key_index;

    bool index_flag = false;
    for(uint i = 0;i < this->index_num;i ++){
        if(this->index[i].key == key_index){
            this->pri_key_index = i;
            index_flag = true;
            break;
        }
    }

    if(!index_flag){
        this->createIndex(key_index);
        this->pri_key_index = index_num - 1;
    }
    return 1;
}

int Sheet::removePrimaryKey(uint key_index) {
    if (this->pri_key == -1)return -1;
    for(int i = 0; i < db->sheet_num;i ++){
        for (uint j = 0;j < db->sheet[i]->col_num;j ++){
            if (db->sheet[i]->col_ty[j].key == Foreign && (uint)db->sheet[i]->col_ty[j].foreign_sheet == sheet_id){
                db->sheet[i]->removeForeignKey(j);
            }
        }
    }
    this->pri_key = -1;
    return 0;
}

int Sheet::createForeignKey(uint key_index, uint sheet_id) {
    if (db->sheet[sheet_id]->pri_key == -1) return -1;
    col_ty[key_index].setForeignKey(sheet_id);
    return 0;
}

int Sheet::removeForeignKey(uint key_index) {
    col_ty[key_index].unsetForeignKey();
    return 0;
}

void Sheet::printCol() {
    std::vector<std::pair<std::string, int> > v;
    v.push_back(std::pair<std::string, int>("Field", 20));
    v.push_back(std::pair<std::string, int>("Type", 15));
    v.push_back(std::pair<std::string, int>("Null", 4));
    v.push_back(std::pair<std::string, int>("Key", 7));
    v.push_back(std::pair<std::string, int>("Default", 20));
    Print::title(v);
    std::vector<Any> d;
    for (uint i = 0; i < col_num; i++) {
        d.push_back(Any((char*)col_ty[i].name));
        switch (col_ty[i].ty) {
        case INT:
            d.push_back(Any((char*)"INT"));
            break;
        case CHAR:
            d.push_back(Any((char*)(("CHAR(" + std::to_string(col_ty[i].char_len) + ")").c_str())));
            break;
        case VARCHAR:
            d.push_back(Any((char*)(("VARCHAR(" + std::to_string(col_ty[i].char_len) + ")").c_str())));
            break;
        case DATE:
            d.push_back(Any((char*)"DATE"));
            break;
        case DECIMAL:
            d.push_back(Any((char*)"DECIMAL"));
            break;
        };
        d.push_back(Any((char*)(col_ty[i].isNull() ? "YES" : "NO")));
        d.push_back(Any((char*)(col_ty[i].key == Primary ? "Primary" : col_ty[i].key == Foreign ? "Foreign" : "")));
        d.push_back(col_ty[i].def);
        Print::row(d);
        d.clear();
    }
    Print::end();
}

void Sheet::print() {
    std::vector<std::pair<std::string, int> > v;
    for (uint i = 0; i < col_num; i++) v.push_back(std::pair<std::string, int>(col_ty[i].name, std::max(col_ty[i].printLen(), (int)strlen(col_ty[i].name))));
    Print::title(v);
    std::vector<Any> d;
    for (uint i = 0; i < record_num; i++) {
        Any* data;
        if (queryRecord(i, data) == 0) {
            for (uint j = 0; j < col_num; j++) d.push_back(data[j]);
        }
        Print::row(d);
        d.clear();
    }
    Print::end();
}
