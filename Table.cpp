#include "Table.h"

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
using namespace std;

#define filterTable(it) ((it) < 0 ? this->db->sel_table[-1-it] : this->db->table[it])

#define exist(buf, offset) (buf[(offset) / 8] & (1 << ((offset) % 8)))
#define exist_set(buf, offset) (buf[(offset) / 8] |= (1 << ((offset) % 8)))
#define exist_reset(buf, offset) (buf[(offset) / 8] -= (1 << ((offset) % 8)))

extern char* dirPath(const char* dir, const char* filename, const char* filetype);
extern char* dirPath(const char* dir, const char* filename, const char* subname, const char* filetype);
extern void replaceFile(const char *oldName, const char *newName);

extern bool cmpCol(enumOp op, Anys &a, Anys &b);

void Table::Pointer::init() {
    pid = -1;
}

bool Table::Pointer::next() {
    pid++;
    if (!s->sel) {
        if (pid % s->record_onepg == 0) {
            buf = s->bpm->getPage(s->main_file, pid / s->record_onepg);
        }
        while ((uint)pid < s->record_num && !exist(buf, pid % s->record_onepg)) {
            pid++;
            if (pid % s->record_onepg == 0) {
                buf = s->bpm->getPage(s->main_file, pid / s->record_onepg);
            }
        }
    }
    return pid < s->record_num;
}

// Anys Table::queryRecord(const int record_id) {
//     if (sel) return this->data[record_id];
//     Anys data;
//     BufType buf = bpm->getPage(main_file, record_id / record_onepg);
//     if (exist(buf, record_id % record_onepg)) {
//         buf += (record_onepg - 1) / 8 + 1;
//         buf += record_size * (record_id % record_onepg);
//         Any any;
//         for (uint i = 0; i < col_num; i++) {
//             fetch(buf, col_ty[i].ty, col_ty[i].size(), any);
//             data.push_back(any);
//         }
//     }
//     return data;
// }

Table::Pointer::Pointer(Table* _s, uint _pid = -1) {
    s = _s;
    pid = _pid;
    if (pid != (uint)-1) buf = s->bpm->getPage(s->main_file, pid / s->record_onepg);
}

Anys Table::Pointer::get() {
    if (s->sel) return s->data[pid];
    BufType _buf = buf + (s->record_onepg - 1) / 8 + 1 + s->record_size * (pid % s->record_onepg);
    Anys as; Any a;
    for (uint i = 0; i < s->col_num; i++) {
        s->fetch(_buf, s->col_ty[i].ty, s->col_ty[i].size(), a);
        as.push_back(a);
    }
    return as;
}

// int Table::queryRecord(const int record_id, Any* &data) {
//     BufType buf = bpm->getPage(main_file, record_id / record_onepg);
//     if (exist(buf, record_id % record_onepg)) {
//         buf += (record_onepg - 1) / 8 + 1;
//         buf += record_size * (record_id % record_onepg);
//         data = new Any[col_num];
//         memset(data, 0, col_num * sizeof(Any));
//         for (uint i = 0; i < col_num; i++) {
//             fetch(buf, col_ty[i].ty, col_ty[i].size(), data[i]);
//         }
//         return 0;
//     } else return -1;
// }

json Table::toJson() { // assemble to JSON
    json j;
    j["name"] = name;
    j["table_id"] = table_id;
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
    for (uint i = 0; i < f_key.size(); i++) j["f_key_index"].push_back(f_key_index[i]);
    return j;
}

Table::Table(Database* db, json j) { // disassemble from JSON
    this->pointer.s = this; 
    this->db = db;
    this->fm = db->fm;
    this->bpm = db->bpm;
    strcpy(name, j["name"].get<std::string>().c_str());
    table_id = j["table_id"].get<int>();
    col_num = j["col_num"].get<int>();
    record_num = j["record_num"].get<int>();
    record_size = j["record_size"].get<int>();
    record_onepg = j["record_onepg"].get<int>();
    for (uint i = 0; i < col_num; i++) col_ty[i] = Type(j["col_ty"][i]);
    fm->openFile(dirPath(db->name, name, "usid"), main_file);
    index_num = j["index_num"].get<int>();
    for (uint i = 0; i < index_num; i++) index[i] = Index(this, j["index"][i]);
    if (j.count("primary")) p_key = new PrimaryKey(this, j["primary"]);
    if (j.count("foreign")) for (uint i = 0; i < j["foreign"].size(); i++) f_key.push_back(new ForeignKey(this, j["foreign"][i], db));
    p_key_index = j["p_key_index"].get<int>();
    f_key_index.clear();
    for(uint i = 0;i < f_key.size();i ++)f_key_index.push_back(j["f_key_index"][i].get<int>()); 
}

Table::~Table() {
    if (sel == false) {
        fm->closeFile(main_file);
        if (p_key) delete p_key;
        for (uint i = 0; i < f_key.size(); i++) delete f_key[i];
    }
}

uint Table::calDataSize() {
    uint size = 0;
    for (uint i = 0; i < col_num; i++) size += col_ty[i].size();
    return size;
}

int Table::createTable(uint table_id, Database* db, const char* name, uint col_num, Type* col_ty, bool create) {
    this->table_id = table_id;
    this->db = db;
    this->fm = db->fm;
    this->bpm = db->bpm;
    strcpy(this->name, name);
    this->col_num = 0;
    vector<uint> tempvec;
    for (uint i = 0; i < col_num; i++) {
        if (this->createColumn(col_ty[i])) return -1;
        if (col_ty[i].key == enumKeyType::Primary)tempvec.push_back(i);
    }

    if(tempvec.size() != 0){
        p_key = new PrimaryKey(this);
        p_key->name = "default primary key name";
        p_key->v.clear();
        for (auto i: tempvec)p_key->v.push_back(i);
        p_key->table = this;
        updateColumns();
        int old = p_key_index;
        if(old != -1)removeIndex(old);
        p_key_index = createIndex(p_key->v, "pk");
        p_key->v_size = p_key->v.size();
    }

    if (create) {
        if (fm->createFile(dirPath(db->name, name, "usid")) == false)
            return -2;
    }
    fm->openFile(dirPath(db->name, name, "usid"), main_file);
    record_size = calDataSize();
    record_onepg = 0;
    while ((record_onepg + 1) * record_size + (record_onepg - 1 + 1) / 8 + 1 <= PAGE_SIZE) record_onepg++;
    return 0;
}

char* Table::getStr(BufType buf, uint size) {
    char* str = new char[size + 1];
    memcpy(str, buf, size);
    str[size] = '\0';
    return str;
}

void Table::insert(Any& val, enumType ty, uint size, BufType& buf) {
    if (val.isNull()) {
        for (uint i = 0; i < size; i++) {
            *(uint8_t*)buf = 255;
            buf += 1;
        }
    }
    else {
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
}

void Table::fetch(BufType& buf, enumType ty, uint size, Any& val) {
    fetchWithOffset(buf, ty, size, val, 0);
    buf += size;
}

int Table::insertRecord(Any* data) {
    int x = this->constraintRow(data, record_num, true);
    if (x) return x;
    int index;
    BufType buf = bpm->getPage(main_file, record_num / record_onepg, index);
    exist_set(buf, record_num % record_onepg);
    buf += (record_onepg - 1) / 8 + 1;
    buf += record_size * (record_num % record_onepg);
    for (uint i = 0; i < col_num; i++) {
        insert(data[i], col_ty[i].ty, col_ty[i].size(), buf);
    }
    for (uint i = 0; i < index_num; i++) {
        Anys a;
        for (uint j = 0; j < this->index[i].key_num; j++) {
            a.push_back(data[this->index[i].key[j]]);
        }
        this->index[i].insertRecord(&a, record_num);
    }
    bpm->markDirty(index);
    record_num++;
    /*
    printf("check index_num:%d record_num:%d\n",index_num,record_num);
    for(uint i = 0;i < record_num;i ++){
        Anys temp = queryRecord(i);
        if (temp[0].isNull()) printf("null ");
        else printf("%d ", *temp[0].anyCast<int>());
        if (temp[1].isNull()) printf("null ");
        else printf("%d ", *temp[1].anyCast<int>());
        printf("\n");
    }*/
    return 0;
}

int Table::removeRecord(const int record_id) {
    int index;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
    if (exist(buf, record_id % record_onepg)) {
        Any* data;
        queryRecord(record_id, data);
        for (uint i = 0; i < index_num; i++) {
            Anys temp;
            for (uint j = 0; j < this->index[i].key_num; j++) {
                temp.push_back(data[this->index[i].key[j]]);
            }
            this->index[i].removeRecord(&temp, record_id);
            if (i == (uint)p_key_index) {
                for (auto _k: p_key->f) {
                    for(uint jx = 0;jx < _k->table->f_key.size();jx ++){
                        if(_k->table->f_key[jx]->p->table == this){
                            std::vector <int> ans;
                            if(_k->table->index[_k->table->f_key_index[jx]].fileID == -1){
                                _k->table->index[_k->table->f_key_index[jx]].open();
                                ans = _k->table->index[_k->table->f_key_index[jx]].queryRecord(&temp);
                                _k->table->index[_k->table->f_key_index[jx]].close();
                            }
                            else {
                                ans = _k->table->index[_k->table->f_key_index[jx]].queryRecord(&temp);
                            }
                            for(uint k = 0;k < ans.size();k ++){
                                if(_k->table->index[_k->table->f_key_index[jx]].fileID == -1){
                                    _k->table->index[_k->table->f_key_index[jx]].open();
                                    _k->table->index[_k->table->f_key_index[jx]].removeRecord(&temp,ans[k]);
                                    Anys nullkey;
                                    for(uint m = 0;m < temp.size();m ++)nullkey.push_back(Any());
                                    _k->table->index[_k->table->f_key_index[jx]].insertRecord(&nullkey,ans[k]);
                                    _k->table->index[_k->table->f_key_index[jx]].close();
                                }
                                else {
                                    _k->table->index[_k->table->f_key_index[jx]].removeRecord(&temp,ans[k]);
                                    Anys nullkey;
                                    for(uint m = 0;m < temp.size();m ++)nullkey.push_back(Any());
                                    _k->table->index[_k->table->f_key_index[jx]].insertRecord(&nullkey,ans[k]);
                                }
                                Anys pre = _k->table->queryRecord(ans[k]);
                                Anys after;
                                vector<bool> mark;
                                for(uint m = 0;m < pre.size();m ++)mark.push_back(true);
                                for(uint m = 0;m < _k->table->f_key[jx]->v.size();m ++)mark[_k->table->f_key[jx]->v[m]] = false;
                                for(uint m = 0;m < pre.size();m ++){
                                    if(mark[m]) after.push_back(pre[m]);
                                    else after.push_back(Any());
                                }
                                int index;
                                BufType buf = bpm->getPage(_k->table->main_file, ans[k] / _k->table->record_onepg, index);
                                buf += (_k->table->record_onepg - 1) / 8 + 1;
                                buf += _k->table->record_size * (ans[k] % _k->table->record_onepg);
                                for (uint q = 0; q < _k->table->col_num; q++) {
                                    insert(after[q], _k->table->col_ty[q].ty, _k->table->col_ty[q].size(), buf);
                                }
                                bpm->markDirty(index);
                            }
                        }
                    }
                }
            }
        }
        
        exist_reset(buf, record_id % record_onepg);
        bpm->markDirty(index);
        return 0;
    }
    return -1;
}

Anys Table::queryRecord(const int record_id) {
    if (sel) return this->data[record_id];
    Anys data;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg);
    if (exist(buf, record_id % record_onepg)) {
        buf += (record_onepg - 1) / 8 + 1;
        buf += record_size * (record_id % record_onepg);
        Any any;
        for (uint i = 0; i < col_num; i++) {
            fetch(buf, col_ty[i].ty, col_ty[i].size(), any);
            data.push_back(any);
        }
    }
    return data;
}

int Table::queryRecord(const int record_id, Any* &data) {
    BufType buf = bpm->getPage(main_file, record_id / record_onepg);
    if (exist(buf, record_id % record_onepg)) {
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

// int Table::updateRecord(const int record_id, const int len, Any* data) {
//     if (this->constraintRow(data, record_id, true)) return -1;
//     int index;
//     BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
//     buf += (record_onepg - 1) / 8 + 1;
//     buf += record_size * (record_id % record_onepg);
//     for (int i = 0; i < len; i++) {
//         insert(data[i], col_ty[i].ty, col_ty[i].size(), buf);
//     }
//     bpm->markDirty(index);
//     return 0;
// }

int Table::updateRecord(const int record_id, vector<Pia> &set) {
    Anys before = queryRecord(record_id);
    Anys after = before;
    for (auto it: set) after[it.first] = it.second;

    int index;
    BufType buf = bpm->getPage(main_file, record_id / record_onepg, index);
    buf += (record_onepg - 1) / 8 + 1;
    buf += record_size * (record_id % record_onepg);
    for (uint i = 0; i < col_num; i++) {
        insert(after[i], col_ty[i].ty, col_ty[i].size(), buf);
    }
    bpm->markDirty(index);

    for (uint i = 0; i < index_num; i++) {
        Anys _b = chooseData(before, this->index[i].key);
        Anys _a = chooseData(after, this->index[i].key);
        if (_b != _a) {
            this->index[i].removeRecord(&_b, record_id);
            this->index[i].insertRecord(&_a, record_id);
        }
    }
    return 0;
}

inline char* getTime() {
    time_t rawtime;
    time( &rawtime );
    char* buffer = new char[MAX_NAME_LEN];
    strftime(buffer, MAX_NAME_LEN, "%Y%m%d%H%M%S", localtime( &rawtime ));
    return buffer;
}

int Table::findIndex(std::string s) {
    for (uint i = 0; i < index_num; i++) 
        if (s == std::string(this->index[i].name)) 
            return i;
    return -1;
}

void Table::fetchWithOffset(BufType& buf, enumType ty, uint size, Any& val, uint offset) {
    buf += offset;
    BufType temp = buf;
    bool flag = true;
    for (uint i = 0; i < size; i++) {
        if (*(uint8_t*)buf != 255) {
            flag = false;
            break;
        }
        buf += 1;
    }
    buf = temp;
    if (flag) val = Any();
    else{
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
    }
    buf -= offset;
}

uint Table::genOffset(uint index) {
    uint ans = 0;
    for (uint i = 0; i < index; i++) {
        ans += this->col_ty[i].size();
    }
    return ans;
}

uint Table::createIndex(vector<uint> col_id, std::string name) {
    //Calculate max per node

    int max_per_node = 25;

    index[index_num] = Index(this, name.c_str(), col_id, max_per_node);
    index[index_num].open();
    for (uint record_id = 0; record_id < record_num; record_id++) {
        BufType buf = bpm->getPage(main_file, record_id / record_onepg);
        if (exist(buf, record_id % record_onepg)) {
            BufType _buf = buf + (record_onepg - 1) / 8 + 1 + record_size * (record_id % record_onepg);
            Anys a;
            for (uint j = 0; j < col_id.size(); j++) {
                Any val;
                fetchWithOffset(_buf, col_ty[index[index_num].key[j]].ty, col_ty[index[index_num].key[j]].size(), val, genOffset(col_id[j]));
                a.push_back(val);
            }
            index[index_num].insertRecord(&a, record_id);
        }
    }
    return index_num++;
}

void Table::removeIndex(uint index_id) {
    index[index_id].close();
    index[index_id].remove();
    index[index_id] = index[--index_num];
}

int Table::createColumn(Type ty) {
    if (record_num > 0 && ty.beNull() == false) return -1;
    col_ty[col_num++] = ty;
    rebuild(1, col_num);
    return 0;
}

int Table::removeColumn(uint col_id) {
    // if (col_ty[col_id].key == Primary) {
    //     printf("Need to remove primary key \"%s\" first.", p_key->name.c_str());
    //     return -1;
    // }
    // for (auto it: f_key) for (auto _i: it->v) if (_i == col_id) {
    //     printf("Need to remove foreign key \"%s\" first.", it->name.c_str());
    //     return -2;
    // }
    // for (uint i = 0; i < index_num; i++) for (auto _i: index[i].key) if (_i == col_id) {
    //     printf("Need to remove index \"%s\" first.", index[i].name);
    //     return -3;
    // }
    rebuild(0, col_id);
    for (uint i = col_id; i < col_num - 1; i++) col_ty[i] = col_ty[i + 1];
    col_num -= 1;

    if (p_key) {
        for (auto it = p_key->v.begin(); it != p_key->v.end(); it++) if (*it == col_id) {
            removePrimaryKey();
            break;
        } else if (*it > col_id) *it -= 1;
    }
    if (f_key.size()) for (uint i = 0; i < f_key.size(); i++) {
        for (auto it = f_key[i]->v.begin(); it != f_key[i]->v.end(); it++) if (*it == col_id) {
            removeForeignKey(f_key[i--]);
            break;
        } else if (*it > col_id) *it -= 1;
    }
    for (uint i = 0; i < index_num; i++) {
        for (auto it = index[i].key.begin(); it != index[i].key.end(); it++) if (*it == col_id) {
            removeIndex(i--);
            break;
        } else if (*it > col_id) *it -= 1;
    }
    return 0;
}

int Table::modifyColumn(uint col_id, Type ty) {
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

void Table::updateColumns() {
    for (uint i = 0; i < col_num; i++) col_ty[i].key = Common;
    if (p_key != nullptr)for (auto it: p_key->v) col_ty[it].key = Primary;
    for (auto _it: f_key) for (auto it: _it->v) col_ty[it].key = Foreign;
}

void Table::rebuild(int ty, uint col_id) {
    if (record_num == 0) return;
    int _main_file;
    fm->createFile(dirPath(db->name, name, "tmp", "usid"));
    fm->openFile(dirPath(db->name, name, "tmp", "usid"), _main_file);
    uint _record_size, _record_onepg;
    int index;
    BufType buf, _buf, buf_st, _buf_st;
    switch (ty) {
    case 0: // del
        _record_size = record_size - col_ty[col_id].size();
        _record_onepg = 0;
        while ((_record_onepg + 1) * _record_size + _record_onepg / 8 + 1 <= PAGE_SIZE) _record_onepg++;
        for (uint i = 0; i < record_num; i++) {
            if (i % record_onepg == 0) {
                buf_st = buf = bpm->getPage(main_file, i / record_onepg);
                buf += (record_onepg - 1) / 8 + 1;
            }
            if (i % _record_onepg == 0) {
                _buf_st = _buf = bpm->getPage(_main_file, i / _record_onepg, index);
                _buf += (_record_onepg - 1) / 8 + 1;
            }
            
            if (exist(buf_st, i % record_onepg))
                exist_set(_buf_st, i % _record_onepg);
            else if (exist(_buf_st, i % _record_onepg))
                exist_reset(_buf_st, i % _record_onepg);
            for (uint j = 0; j < col_num; j++) {
                if (j != col_id) memcpy(_buf, buf, col_ty[j].size());
                buf += col_ty[j].size();
                if (j != col_id) _buf += col_ty[j].size();
            }
            bpm->markDirty(index);
        }
        break;
    case 1: // add
        _record_size = record_size + col_ty[col_id].size();
        _record_onepg = 0;
        while ((_record_onepg + 1) * _record_size + _record_onepg / 8 + 1 <= PAGE_SIZE) _record_onepg++;
        for (uint i = 0; i < record_num; i++) {
            if (i % record_onepg == 0) {
                buf_st = buf = bpm->getPage(main_file, i / record_onepg);
                buf += (record_onepg - 1) / 8 + 1;
            }
            if (i % _record_onepg == 0) {
                _buf_st = _buf = bpm->getPage(_main_file, i / _record_onepg, index);
                _buf += (_record_onepg - 1) / 8 + 1;
            }
            if (exist(buf_st, i % record_onepg)) 
                exist_set(_buf_st, i % _record_onepg);
            else if (exist(_buf_st, i % _record_onepg))
                exist_reset(_buf_st, i % _record_onepg);
            for (uint j = 0; j < col_num - 1; j++) {
                memcpy(_buf, buf, col_ty[j].size());
                buf += col_ty[j].size(); 
                _buf += col_ty[j].size();
            }
            insert(col_ty[col_num - 1].def, col_ty[col_num - 1].ty, col_ty[col_num - 1].size(), _buf);
            bpm->markDirty(index);
        }
        break;
    }
    fm->closeFile(main_file);
    fm->closeFile(_main_file);
    replaceFile(dirPath(db->name, name, "tmp", "usid"), dirPath(db->name, name, "usid"));

    fm->openFile(dirPath(db->name, name, "usid"), main_file);
    record_size = _record_size;
    record_onepg = _record_onepg;
}

int Table::createPrimaryKey(PrimaryKey* pk) {
    if (p_key) return -1;
    if (constraintKey(pk)) return -2;
    p_key = new PrimaryKey(this);
    p_key->v_size = pk->v_size;
    p_key->name = pk->name;
    p_key->v.clear();
    for (auto i: pk->v)p_key->v.push_back(i);
    p_key->table = pk->table;
    updateColumns();
    if (p_key_index != -1) removeIndex(p_key_index);
    p_key_index = createIndex(p_key->v, "pk");
    return 0;
}

int Table::removePrimaryKey() {
    if (p_key == nullptr) return -1;
    for (auto it: p_key->f) it->p = nullptr;
    for (auto col: p_key->v) col_ty[col].key = enumKeyType::Common;
    int index_id = p_key_index;
    removeIndex(index_id);
    delete p_key;
    p_key = nullptr;
    return 0;
}

int Table::createForeignKey(ForeignKey* fk, PrimaryKey* pk) {
    if (*pk->table->p_key != *pk) return -1;
    fk->p = pk->table->p_key;
    if (fk->size() != pk->size()) return -3;
    for (uint i = 0; i < fk->size(); i++) {
        if (col_ty[fk->v[i]].ty == CHAR || col_ty[fk->v[i]].ty == VARCHAR) {
            if (pk->table->col_ty[pk->v[i]].ty != CHAR && pk->table->col_ty[pk->v[i]].ty != VARCHAR) return -5;
        }
        else if (col_ty[fk->v[i]].ty != pk->table->col_ty[pk->v[i]].ty) return -5;
    }
    if (constraintKey(fk)) return -2;
    
    ForeignKey* f_keyx = new ForeignKey(this);
    f_keyx->v_size = fk->v_size;
    f_keyx->name = fk->name;
    f_keyx->v.clear();
    for (auto i: fk->v)f_keyx->v.push_back(i);
    f_keyx->table = fk->table;
    f_keyx->p = pk->table->p_key;
    f_key.push_back(f_keyx);
    updateColumns();

    string name = std::string("fk_") + std::string(f_keyx->name) + std::to_string(f_key.size());
    f_key_index.push_back(createIndex(f_keyx->v, name));

    pk->table->p_key->f.push_back(f_keyx);

    return 0;
}

int Table::removeForeignKey(ForeignKey* fk) {
    int i = 0;
    for (auto it = f_key.begin(); it != f_key.end(); it++){
        if (*it == fk) { 
            for (auto _it = fk->p->f.begin(); _it != fk->p->f.end(); _it++) if (*_it == fk) {
                removeIndex(f_key_index[i]);
                f_key_index.erase(f_key_index.begin()+i);
                f_key.erase(it);
                updateColumns();
                fk->p->f.erase(_it);
                delete fk;
                return 0;
            }
            return -1;
        }
        i ++;
    }
    return -1;
}

void Table::print() {
    std::vector<std::pair<std::string, int> > v;
    for (uint i = 0; i < col_num; i++) v.push_back(std::pair<std::string, int>(col_ty[i].name, std::max(col_ty[i].printLen(), (int)strlen(col_ty[i].name))));
    Print::title(v);
    if (sel == 1) {
        for (auto it: data) {
            Print::row(it);
        }
    } else if (sel == 2) {
        Print::row(val);
    } else {
        Pointer p(this);
        while (p.next()) Print::row(p.get());
    }
    Print::end();
}

void Table::printCol() {
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
        d.push_back(Any((char*)(col_ty[i].beNull() ? "YES" : "NO")));
        d.push_back(Any((char*)(col_ty[i].key == Primary ? "Primary" : col_ty[i].key == Foreign ? "Foreign" : "")));
        d.push_back(col_ty[i].def);
        Print::row(d);
        d.clear();
    }
    Print::end();
}

bool Table::printBack(uint num) {
    if (num == 0 || sel != 1) return false;
    std::vector<std::pair<std::string, int> > v;
    for (uint i = 0; i < col_num; i++) v.push_back(std::pair<std::string, int>(col_ty[i].name, std::max(col_ty[i].printLen(), (int)strlen(col_ty[i].name))));
    Print::title(v);
    for (uint it = record_num - num; it < record_num; it++) { 
        Print::row(data[it]);
    }
    Print::end();
    return true;
}

int Table::findCol(std::string a) {
    for (uint i = 0; i < col_num; i++) if (a == std::string(this->col_ty[i].name)) return i;
    return -1;
}

int Table::constraintCol(uint col_id) {
    if (col_ty[col_id].beNull() == false) {
        Pointer p(this);
        while (p.next()) if (p.get()[col_id].isNull()) return -1;
    }
    return 0;
}

int Table::constraintKey(Key* key) {
    Anys data, _data;
    if (key->ty() == 1) {
        std::map<Anys, bool> m;
        Pointer p(this);
        while (p.next()) {
            data = p.get();
            for (auto i: key->v) {
                if (data[i].isNull()) return -1;
                _data.push_back(data[i]);
            }
            if (m.count(_data)) return -2;
            m[_data] = true;
        }
        // for (uint record_id = 0; record_id < record_num; record_id++) {
        //     if (queryRecord(record_id, data)) continue;
        //     Anys as;
        //     for (auto i: key->v) {
        //         if (data[i].isNull()) return -1;
        //         as.push_back(data[i]);
        //     }
        //     if (m.count(as)) return -2;
        //     m[as] = true;
        // }
    } else {
        Pointer p(this);
        while (p.next()) {
            data = p.get();
            Anys _data;
            bool null = true, non_null = true;
            for (auto i: key->v) {
                null &= data[i].isNull();
                non_null &= !data[i].isNull();
                _data.push_back(data[i]);
            }
            if (null == false && non_null == false) return -1;
            Table* p_table = dynamic_cast<ForeignKey*>(key)->p->table;
            if (p_table->index[p_table->p_key_index].queryRecord(&_data).size() == 0) return -4;
        }
        // for (uint record_id = 0; record_id < record_num; record_id++) {
        //     if (queryRecord(record_id, data)) continue;
        //     Anys as;
        //     bool null = true, non_null = true;
        //     for (auto i: key->v) {
        //         null &= data[i].isNull();
        //         non_null &= !data[i].isNull();
        //         as.push_back(data[i]);
        //     }
        //     if (null == false && non_null == false) return -1;
        //     Anys _data = Anys(data, col_num);
        //     Anys val = chooseData(_data, key->v);
        //     Table* p_table = dynamic_cast<ForeignKey*>(key)->p->table;
        //     if (p_table->index[p_table->p_key_index].queryRecord(&val).size() == 0) return -4;
        // }
    }
    return 0;
}

int Table::constraintRow(Any* data, uint record_id, bool ck_unique) {
    for (uint i = 0; i < col_num; i++) {
        if (col_ty[i].beNull() == false && data[i].isNull()) return -1;
    }
    if (p_key != nullptr && constraintRowKey(data, p_key)) return -3;
    for (auto it: f_key) if (constraintRowKey(data, it)) return -4;
    return 0;
}

Anys Table::chooseData(Anys& data, vector<uint>& cols) {
    Anys val;
    for (auto i: cols) val.push_back(data[i]);
    return val;
}

int Table::constraintRowKey(Any* data, Key* key) {
    if (key->ty() == 1) {
        for (auto i: key->v) if (data[i].isNull()) return -1;
        Anys val;
        for (auto i: key->v) val.push_back(data[i]);
        vector<int> ans;
        if(this->index[p_key_index].fileID == -1){
            this->index[p_key_index].open();
            ans = this->index[p_key_index].queryRecord(&val);
            this->index[p_key_index].close();
        }
        else ans = this->index[p_key_index].queryRecord(&val);
        if (ans.size() != 0) return -2;
    } else {
        Anys val;
        for (auto i: key->v) val.push_back(data[i]);
        vector <int> ans;
        //if(key->v.size() == 2)printf("%d %d\n",*val[0].anyCast<int>(),*val[1].anyCast<int>());
        Table* p_table = dynamic_cast<ForeignKey*>(key)->p->table;
        if(p_table->index[p_table->p_key_index].fileID == -1){
            p_table->index[p_table->p_key_index].open();
            ans = p_table->index[p_table->p_key_index].queryRecord(&val);
            p_table->index[p_table->p_key_index].close();
        }
        else {ans = p_table->index[p_table->p_key_index].queryRecord(&val);}
        if (ans.size() == 0){
            return -3;
        }
    }
    return 0;
}

void Table::initPointer() {
    return pointer.init();
}

bool Table::movePointer() {
    return pointer.next();
}

Any Table::getPointerColData(uint idx) {
    return pointer.get()[idx];
}

Anys Table::getPointerData() {
    return pointer.get();
}

bool Table::cmpRecord(Anys a, Anys b, enumOp op) {
    switch (op) {
    case OP_EQ: return a == b;
    case OP_NEQ: return a != b;
    case OP_LE: return a <= b;
    case OP_LS: return a < b;
    case OP_GE: return a >= b;
    case OP_GT: return a > b;
    default: return false;
    }
}

bool Table::cmpRecords(Anys data, enumOp op, bool any, bool all) {
    bool cmp;
    Anys _data;
    Pointer p(this);
    while (p.next()) {
        _data = p.get();
        cmp = cmpRecord(data, _data, op);
        if (any && cmp) return true;
        if (all && !cmp) return false;
    }
    // for (uint record_id = 0; record_id < record_num; record_id++) {
    //     if ((_data = queryRecord(record_id)).size() == 0) continue;
    //     cmp = cmpRecord(data, _data, op);
    //     if (any && cmp) return true;
    //     if (all && !cmp) return false;
    // }
    if (any) return false;
    if (all) return true;
    return false;
}

bool Table::checkWhere(Anys __data, WhereStmt &w) { // Database::checkWhere
    Anys data, _data;
    for (auto it: w.cols) data.push_back(__data[it.second]);
    switch (w.rvalue_ty) {
    case 0: {
        bool nl = true, nnl = true;
        for (uint i = 0; i < w.cols.size(); i++) {
            nl &= data[i].isNull();
            nnl &= !data[i].isNull();
        }
        return (w.op == OP_NL && nl) && (w.op == OP_NNL && nnl);
    }
    case 1: {
        for (auto it: w.rvalue) {
            _data.push_back(it);
        }
        return cmpCol(w.op, data, _data);
    }
    case 2: {
        for (auto it: w.rvalue_cols) {
            _data.push_back(__data[it.second]);
        }
        return cmpCol(w.op, data, _data);
    }
    case 3:
        if (w.op == OP_IN) w.any = true, w.op = OP_EQ;
        if (w.any || w.all) {
            return filterTable(w.rvalue_table)->cmpRecords(data, w.op, w.any, w.all);
        } else {
            return filterTable(w.rvalue_table)->cmpRecord(data, filterTable(w.rvalue_table)->val, w.op);
        }
    default:
        return false;
    }
}

bool Table::checkWheres(Anys data, vector<WhereStmt> &where) {
    for (auto it: where) if (checkWhere(data, it) == false) return false;
    return true;
}

vector<uint> Table::existentRecords() {
    vector<uint> ret;
    BufType buf;
    for (uint record_id = 0; record_id < record_num; record_id++) {
        if (record_id % record_onepg == 0) {
            buf = bpm->getPage(main_file, record_id / record_onepg);
        }
        if (exist(buf, record_id % record_onepg)) {
            ret.push_back(record_id);
        }
    }
    return ret;
}

vector<uint> Table::findWheres(vector<WhereStmt> &where) { // preliminary search
    int mx = -1;
    WhereStmt w;
    // uint w_index;
    for (auto it: where) if (it.rvalue_ty == 1) {
        for (uint i = 0; i < index_num; i++) if (index[i].key.size() == it.cols.size()) {
            bool tmp = true;
            for (uint j = 0; j < it.cols.size(); j++) tmp &= (index[i].key[j] == it.cols[j].second);
            if (tmp) {
                Anys data;
                for (auto _it: it.rvalue) data.push_back(_it);
                int nums = index[i].queryRecordsNum(it.op, data);
                if (nums > mx) {
                    mx = nums;
                    w = it;
                    // w_index = i;
                }
            }
        }
    }

    // FIXME
    // if (mx == -1) {
        return existentRecords();
    // } else {
    //     Anys data;
    //     for (auto it: w.rvalue) data.push_back(it);
    //     return index[w_index].queryRecords(w.op, data);           
    // }
}

int Table::removeRecords(vector<WhereStmt> &where) {
    vector<uint> cols = findWheres(where);
    for (auto i: cols) {
        Pointer p(this, i);
        if (checkWheres(p.get(), where)) {
            removeRecord(i);
        }
    }
    return 0;
}

int Table::updateRecords(vector<Pia> &set, vector<WhereStmt> &where) {
    for (auto it: set) {
        if (col_ty[it.first].beNull() == false && it.second.isNull() == true) return -1;
    }
    vector<uint> _cols = findWheres(where), cols;
    vector<Anys> pk_add, pk_del, _pk_add, _pk_del;
    for (auto i: _cols) {
        Pointer p(this, i);
        Anys data = p.get();
        if (checkWheres(data, where)) {
            bool modify = false;
            for (auto it: set) for (auto _i: p_key->v) if (_i == (uint)it.first) {
                modify = true;
                goto BK1;
            }
            BK1: if (modify) pk_del.push_back(chooseData(data, p_key->v));
            for (auto it: set) data[it.first] = it.second;
            if (modify) pk_add.push_back(chooseData(data, p_key->v));
            for (auto it: f_key) {
                for (auto _it: set) for (auto _i: it->v) if (_i == (uint)_it.first) goto BK2;
                continue;
                BK2: Anys val = chooseData(data, it->v);
                vector<int> ans;
                Table* p_table = it->p->table;
                ans = p_table->index[p_table->p_key_index].queryRecord(&val);
                printf("%d %d\n", val[0].anyRefCast<int>(), val.size());
                if (ans.size() == 0) goto FAIL;
            }
            cols.push_back(i);
            FAIL:;
        }
    }

    sort(pk_add.begin(), pk_add.end());
    sort(pk_del.begin(), pk_del.end());
    vector<Anys>::iterator add_it = pk_add.begin(), del_it = pk_del.begin(), it;
    while (add_it != pk_add.end() && del_it != pk_del.end()) {
        if (*add_it == *del_it) add_it++, del_it++; 
        else if (*add_it < *del_it) _pk_add.push_back(*add_it), add_it++;
        else _pk_del.push_back(*del_it), del_it++;
    }
    while (add_it != pk_add.end()) _pk_add.push_back(*add_it), add_it++;
    while (del_it != pk_del.end()) _pk_del.push_back(*del_it), del_it++;

    for (uint i = 1; i < _pk_add.size(); i++) if (_pk_add[i-1] == _pk_add[i]) return -5;
    for (uint i = 0; i < _pk_add.size(); i++) {
        vector<int> ans = this->index[p_key_index].queryRecord(&_pk_add[i]);
        if (ans.size() != 0) return -7;
    }
    
    for (auto i: cols) updateRecord(i, set);
    for (auto it: pk_del) {
        for (uint i = 0;i < db->table_num;i ++){
            for(uint j = 0;j < db->table[i]->f_key.size();j ++){
                if(db->table[i]->f_key[j]->p->table == this){
                    std::vector <int> ans;
                    if(db->table[i]->index[db->table[i]->f_key_index[j]].fileID == -1){
                        db->table[i]->index[db->table[i]->f_key_index[j]].open();
                        ans = db->table[i]->index[db->table[i]->f_key_index[j]].queryRecord(&it);
                        db->table[i]->index[db->table[i]->f_key_index[j]].close();
                    }
                    else {
                        ans = db->table[i]->index[db->table[i]->f_key_index[j]].queryRecord(&it);
                    }
                    for(uint k = 0;k < ans.size();k ++){
                        if(db->table[i]->index[db->table[i]->f_key_index[j]].fileID == -1){
                            db->table[i]->index[db->table[i]->f_key_index[j]].open();
                            db->table[i]->index[db->table[i]->f_key_index[j]].removeRecord(&it,ans[k]);
                            Anys nullkey;
                            for(uint m = 0;m < it.size();m ++)nullkey.push_back(Any());
                            db->table[i]->index[db->table[i]->f_key_index[j]].insertRecord(&nullkey,ans[k]);
                            db->table[i]->index[db->table[i]->f_key_index[j]].close();
                        }
                        else {
                            db->table[i]->index[db->table[i]->f_key_index[j]].removeRecord(&it,ans[k]);
                            Anys nullkey;
                            for(uint m = 0;m < it.size();m ++)nullkey.push_back(Any());
                            db->table[i]->index[db->table[i]->f_key_index[j]].insertRecord(&nullkey,ans[k]);
                        }
                        Anys pre = db->table[i]->queryRecord(ans[k]);
                        Anys after;
                        vector<bool> mark;
                        for(uint m = 0;m < pre.size();m ++)mark.push_back(true);
                        for(uint m = 0;m < db->table[i]->f_key[j]->v.size();m ++)mark[db->table[i]->f_key[j]->v[m]] = false;
                        for(uint m = 0;m < pre.size();m ++){
                            if(mark[m]) after.push_back(pre[m]);
                            else after.push_back(Any());
                        }
                        int index;
                        BufType buf = bpm->getPage(main_file, ans[k] / record_onepg, index);
                        buf += (record_onepg - 1) / 8 + 1;
                        buf += record_size * (ans[k] % record_onepg);
                        for (uint i = 0; i < col_num; i++) {
                            insert(after[i], col_ty[i].ty, col_ty[i].size(), buf);
                        }
                        bpm->markDirty(index);
                    }
                }
            }
        }
    }
    return 0;
}

uint Table::getPointer() {
    return pointer.pid;
}
