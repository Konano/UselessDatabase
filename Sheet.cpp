#include "Sheet.h"

#include "Database.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include "Type.h"
#include "Index.h"
#include "Print.h"

#include <vector>
#include <map>

#include <time.h>
#include <iostream>
using namespace std; // TODO remove

#define exist(buf, offset) (buf[(offset) / 8] & (1 << ((offset) % 8))) // TODO modify all

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
    if (p_key) j["primary"] = p_key->toJson();
    for (auto it: f_key) j["foreign"].push_back(it->toJson());
    j["p_key_index"] = p_key_index;
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
    if (j.count("primary")) p_key = new PrimaryKey(this, j["primary"]);
    if (j.count("foreign")) for (uint i = 0; i < j["foreign"].size(); i++) f_key.push_back(new ForeignKey(this, j["foreign"][i], db));
    p_key_index = j["p_key_index"];
}

Sheet::~Sheet() {
    if (sel == false) {
        fm->closeFile(main_file);
        if (p_key) delete p_key;
        for (uint i = 0; i < f_key.size(); i++) delete f_key[i];
    }
}

uint Sheet::calDataSize() {
    uint size = 0;
    for (uint i = 0; i < col_num; i++) size += col_ty[i].size();
    return size;
}

int Sheet::createSheet(uint sheet_id, Database* db, const char* name, uint col_num, Type* col_ty, bool create) {
    this->sheet_id = sheet_id;
    this->db = db;
    this->fm = db->fm;
    this->bpm = db->bpm;
    strcpy(this->name, name);
    this->col_num = 0;
    for (uint i = 0; i < col_num; i++) {
        if (this->createColumn(col_ty[i])) return -1;
    }
    if (create) {
        if (fm->createFile(dirPath(db->name, name, ".usid")) == false)
            return -2;
    }
    fm->openFile(dirPath(db->name, name, ".usid"), main_file);
    record_size = calDataSize();
    record_onepg = 0;
    while ((record_onepg + 1) * record_size + record_onepg / 8 + 1 <= PAGE_SIZE) record_onepg++;
    return 0;
}

char* Sheet::getStr(BufType buf, uint size) {
    char* str = new char[size + 1];
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
        memcpy(buf, *val.anyCast<char*>(), strlen(*val.anyCast<char*>()));
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
        val = *(uint32_t*)buf; 
        break;
    case DECIMAL: 
        val = *(long double*)buf; 
        break;
    }
    buf += size;
}

int Sheet::insertRecord(Any* data) {
    if (this->constraintRow(data, record_num, true)) return -1;
    int index;
    BufType buf = bpm->getPage(main_file, record_num / record_onepg, index);
    buf[(record_num % record_onepg) / 8] |= 1 << (record_num % 8);
    buf += (record_onepg - 1) / 8 + 1;
    buf += record_size * (record_num % record_onepg);
    for (uint i = 0; i < col_num; i++) {
        insert(data[i], col_ty[i].ty, col_ty[i].size(), buf);
    }
    for (uint i = 0; i < index_num; i++) {
        this->index[i].insertRecord(&data[this->index[i].key], record_num);
    }
    bpm->markDirty(index);
    record_num++;
    return 0;
}

int Sheet::removeRecord(const int record_id) { // TODO when remove some p_key data, need to check f_key...
    int index;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
    if (buf[(record_id % record_onepg) / 8] & (1 << (record_id % 8))) {
        Any* data;
        queryRecord(record_id, data);
        for (uint i = 0; i < index_num; i++) {
            this->index[i].removeRecord(&data[this->index[i].key], record_id);
        }
        buf[(record_id % record_onepg) / 8] -= 1 << (record_id % 8);
        bpm->markDirty(index);
        return 0;
    }
    return -1;
}

int Sheet::queryRecord(const int record_id, Any* &data) {
    int index;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
    if (buf[(record_id % record_onepg) / 8] & (1 << (record_id % 8))) {
        buf += (record_onepg - 1) / 8 + 1;
        buf += record_size * (record_id % record_onepg);
        data = new Any[col_num];
        memset(data, 0, col_num * sizeof(Any));
        for (uint i = 0; i < col_num; i++) {
            fetch(buf, col_ty[i].ty, col_ty[i].size(), data[i]);
        }
        return 0;
    } else return -1;
}

int Sheet::updateRecord(const int record_id, const int len, Any* data) { // TODO when update some p_key data, need to check f_key... // TODO need to modify index
    if (this->constraintRow(data, record_id, true)) return -1;
    int index;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
    buf += (record_onepg - 1) / 8 + 1;
    buf += record_size * (record_id % record_onepg);
    for (int i = 0; i < len; i++) {
        insert(data[i], col_ty[i].ty, col_ty[i].size(), buf);
    }
    bpm->markDirty(index);
    return 0;
}

// bool Sheet::queryPrimaryKey(Any* info) {
//     if (p_key == nullptr) return false;
//     // TODO
//     int ans = ;
//     return ans != -1;
//     int _index;
//     BufType buf, buf_st;
//     int size_pre = 0;
//     for (int i = 0; i < p_key; i++)
//         size_pre += col_ty[i].size();
//     for (uint i = 0; i < record_num; i++) {  // TODO i -> record_id
//         if (i % record_onepg == 0) {
//             buf_st = buf = bpm->getPage(main_file, i / record_onepg, _index);
//             buf += (record_onepg - 1) / 8 + 1;
//         } else { buf += record_size; }
//         if (buf_st[(i % record_onepg) / 8] & (1 << (i % 8))) {
//             buf += size_pre;
//             switch (this->col_ty[p_key].ty) {
//             case INT:
//                 if (Any(*(int*)buf) == query_val) return true;
//                 break;
//             case CHAR:
//                 if (Any(getStr(buf, col_ty[p_key].size())) == query_val) return true;
//                 break;
//             case VARCHAR:
//                 if (Any(db->getVarchar(*(uint64_t*)buf)) == query_val) return true;
//                 break;
//             case DATE:
//                 if (Any(*(int*)buf) == query_val) return true;
//                 break;
//             case DECIMAL:
//                 if (Any(*(long double*)buf) == query_val) return true;
//                 break;
//             }
//             buf -= size_pre;
//         }
//     }
//     return false;
// }

inline char* getTime() {
    time_t rawtime;
    time( &rawtime );
    char* buffer = new char[MAX_NAME_LEN];
    strftime(buffer, MAX_NAME_LEN, "%Y%m%d%H%M%S", localtime( &rawtime ));
    return buffer;
}

uint Sheet::createIndex(uint key_index) {
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
    return index_num++;
}

void Sheet::removeIndex(uint index_id) {
    index[index_id].close();
    index[index_id].remove();
    index[index_id] = index[--index_num];
}

int Sheet::createColumn(Type ty) {
    if (record_num > 0 && ty.isNull() == false) return -1;
    col_ty[col_num++] = ty;
    rebuild(1, col_num);
    return 0;
}

int Sheet::removeColumn(uint col_id) { // TODO need to modify index
    rebuild(0, col_id);
    for (uint i = col_id; i < col_num - 1; i++) col_ty[i] = col_ty[i + 1];
    col_num -= 1;

    if (p_key) {
        for (auto it = p_key->v.begin(); it != p_key->v.end(); it++) if (*it == col_id) {
            removePrimaryKey();
            break;
        } else if (*it > col_id) *it -= 1;
    }
    if (f_key.size()) for(uint i = 0; i < f_key.size(); i++) {
        for (auto it = f_key[i]->v.begin(); it != f_key[i]->v.end(); it++) if (*it == col_id) {
            removeForeignKey(f_key[i--]);
            break;
        } else if (*it > col_id) *it -= 1;
    }
    return 0;
}

int Sheet::modifyColumn(uint col_id, Type ty) { // TODO 对于 Key 和 index 的维护
    switch (col_ty[col_id].ty) {
    case INT: 
    case DATE: 
    case DECIMAL:
        if (ty.ty != col_ty[col_id].ty) return -1;
        break;
    case CHAR:
    case VARCHAR:
        if (ty.ty == CHAR || ty.ty == VARCHAR) {
            if (col_ty[col_id].char_len > ty.char_len) return -2;
        } else return -1;
        break;
    }
    Type tmp = col_ty[col_id];
    col_ty[col_id] = ty;
    col_ty[col_id].key = tmp.key;
    if (constraintCol(col_id)) {
        col_ty[col_id] = tmp;
        return -3;
    }
    return 0;
}

void Sheet::updateColumns() {
    for (uint i = 0; i < col_num; i++) col_ty[i].key = Common;
    for (auto it: p_key->v) col_ty[it].key = Primary;
    for (auto _it: f_key) for (auto it: _it->v) col_ty[it].key = Foreign;
}

void Sheet::rebuild(int ty, uint key_index) { // TODO rebuild index
    if (record_num == 0) return;
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
            for (uint j = 0; j < col_num; j++) {
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
            for (uint j = 0; j < col_num - 1; j++) {
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

int Sheet::createPrimaryKey(PrimaryKey* pk) {
    if (p_key) return -1;
    if (constraintKey(pk)) return -2;
    p_key = pk;
    updateColumns();
    // TODO new p_key_index
    return 0;

    // std::vector<Any> v;

    // int _index;
    // BufType buf, buf_st;
    // for (uint record_id = 0; record_id < record_num; record_id++) {
    //     if (record_id % record_onepg == 0) {
    //         buf = buf_st = bpm->getPage(main_file, record_id / record_onepg, _index);
    //         buf += (record_onepg - 1) / 8 + 1;
    //     } else { buf += record_size; }
    //     if (buf_st[(record_id % record_onepg) / 8] & (1 << (record_id % 8))) {

    //         Any* info = (Any*) malloc(col_num * sizeof(Any));
    //         memset(info, 0, col_num * sizeof(Any));
    //         for (uint i = 0; i < col_num; i++) {
    //             fetch(buf, col_ty[i].ty, col_ty[i].size(), info[i]);
    //         }
    //         bool flag = false;
    //         for (auto a: v){
    //             if (a == info[key_index])
    //             {
    //                 flag = true;
    //                 break;
    //             }
    //         }
    //         if (!flag) v.push_back(info[key_index]);
    //         else return -1;
    //         //index[index_num].insertRecord(&info[index[index_num].key], record_id);
    //     }
    // }

    // this->p_key.push_back(key_index);

    // bool index_flag = false;
    // for (uint i = 0;i < this->index_num;i ++){
    //     if(this->index[i].key == key_index){
    //         this->p_key_index = i;
    //         index_flag = true;
    //         break;
    //     }
    // }

    // if(!index_flag){
    //     this->createIndex(key_index);
    //     this->p_key_index = index_num - 1;
    // }
    // return 1;
}

int Sheet::removePrimaryKey() {
    if (p_key == nullptr) return -1;
    for (auto it: p_key->f) it->p = nullptr;
    // TODO delete p_key_index
    delete p_key;
    p_key = nullptr;
    return 0;
}

int Sheet::createForeignKey(ForeignKey* fk, PrimaryKey* pk) {
    if (*pk->sheet->p_key != *pk) return -1;
    fk->p = pk->sheet->p_key;
    if (fk->size() != pk->size()) return -3;
    for (uint i = 0; i < fk->size(); i++) {
        if (col_ty[fk->v[i]].ty == CHAR || col_ty[fk->v[i]].ty == VARCHAR) {
            if (pk->sheet->col_ty[pk->v[i]].ty != CHAR && pk->sheet->col_ty[pk->v[i]].ty != VARCHAR) return -5;
        }
        else if (col_ty[fk->v[i]].ty != pk->sheet->col_ty[pk->v[i]].ty) return -5;
    }
    if (constraintKey(fk)) return -2;
    f_key.push_back(fk);
    updateColumns();
    pk->sheet->p_key->f.push_back(fk);
    return 0;
}

int Sheet::removeForeignKey(ForeignKey* fk) {
    for (auto it = f_key.begin(); it != f_key.end(); it++) if (*it == fk) { 
        for (auto _it = fk->p->f.begin(); _it != fk->p->f.end(); _it++) if (*_it == fk) {
            f_key.erase(it);
            updateColumns();
            fk->p->f.erase(_it);
            delete fk;
            return 0;
        }
        return -1;
    }
    return -1;
}

void Sheet::printCol() {
    std::vector<std::pair<std::string, int> > v;
    v.push_back(std::pair<std::string, int>("Field", 20));
    v.push_back(std::pair<std::string, int>("Type", 15));
    v.push_back(std::pair<std::string, int>("Null", 4));
    v.push_back(std::pair<std::string, int>("Key", 7));
    v.push_back(std::pair<std::string, int>("Default", 20));
    Print::title(v);
    Anys d;
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
    if (sel) {
        for (auto it: data) {
            Print::row(it);
        }
    } else {
        Anys d;
        for (uint i = 0; i < record_num; i++) {
            Any* data;
            if (queryRecord(i, data) == 0) {
                for (uint j = 0; j < col_num; j++) d.push_back(data[j]);
                Print::row(d);
                d.clear();
            }
        }
    }
    Print::end();
}

int Sheet::constraintCol(uint col_id) {
    Any* data;
    std::map<Any, bool> m;
    for (uint record_id = 0; record_id < record_num; record_id++) {
        if (queryRecord(record_id, data)) continue;
        if (col_ty[col_id].isNull() == false && data[col_id].isNull()) return -1;
    }
    return 0;
}

int Sheet::constraintKey(Key* key) {
    if (key->ty() == 1) {
        Any* data;
        std::map<Anys, bool> m;
        for (uint record_id = 0; record_id < record_num; record_id++) {
            if (queryRecord(record_id, data)) continue;
            Anys as;
            for (auto i: key->v) {
                if (data[i].isNull()) return -1;
                as.push_back(data[i]);
            }
            if (m.count(as)) return -2;
            m[as] = true;
        }
    } else {
        Any* data;
        for (uint record_id = 0; record_id < record_num; record_id++) {
            if (queryRecord(record_id, data)) continue;
            Anys as;
            bool null = true, non_null = true;
            for (auto i: key->v) {
                null &= data[i].isNull();
                non_null &= !data[i].isNull();
                as.push_back(data[i]);
            }
            if (null == false && non_null == false) return -1;
            // TODO check exist(find p_key_index)
        }
    }
    return 0;
}

int Sheet::constraintRow(Any* data, uint record_id, bool ck_unique) {
    for (uint i = 0; i < col_num; i++) {
        if (col_ty[i].isNull() == false && data[i].isNull()) return -1;
    }
    if (constraintRowKey(data, p_key)) return -3;
    for (auto it: f_key) if (constraintRowKey(data, it)) return -4;
    return 0;
}

int Sheet::constraintRowKey(Any* data, Key* key) {
    if (key->ty() == 1) {
        for (auto i: key->v) if (data[i].isNull()) return -1;
        // TODO check unique(find p_key_index)
    } else {
        // TODO check exist(find p_key_index)
    }
    return 0;
}
    
int Sheet::findCol(std::string s) {
    for (uint i = 0; i < col_num; i++) if (std::string(col_ty[i].name) == s) return i;
    return -1;
}

void Sheet::setPointer(int pointer) {
    this->pointer = pointer;
}

bool Sheet::movePointer() {
    pointer++;
    if (!sel) {
        int index;
        BufType buf = bpm->getPage(main_file, pointer / record_onepg, index);
        while ((uint)pointer < record_num && !exist(buf, pointer % record_onepg)) {
            pointer++;
            if (pointer % record_onepg == 0) {
                buf = bpm->getPage(main_file, pointer / record_onepg, index);
            }
        }
    }
    return (uint)pointer < record_num;
}

Any Sheet::getPointerColData(uint idx) {
    if (sel) {
        return data[pointer][idx];
    } else {
        Any* data;
        queryRecord(pointer, data);
        return data[idx]; // TODO Anys
    }
}

Anys Sheet::getPointerData() {
    if (sel) {
        return data[pointer];
    } else {
        Any* data;
        queryRecord(pointer, data);
        return Anys(data, col_num); // TODO Anys
    }
}

// TODO Key 名字，还不能重复

// TODO Foreign Key 需要索引吗