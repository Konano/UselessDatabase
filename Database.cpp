#include "Database.h"

#include "Sheet.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include "Print.h"

#include <vector>
#include <string>

#ifdef WIN32
#include <direct.h>
#elif __linux__
#include <sys/types.h>
#include <sys/stat.h>
#endif

#define filterSheet(it) ((it) < 0 ? sel_sheet[-1-it] : sheet[it])

extern char* readFile(const char* path);
extern void writeFile(const char* path, const char* data, const int length);
extern char* dirPath(const char* dir, const char* path);
extern char* dirPath(const char* dir, const char* filename, const char* suffix);

json Database::toJson() {
    json j;
    j["name"] = name;
    j["sheet_num"] = sheet_num;
    j["mem"] = mem;
    for (uint i = 0; i < sheet_num; i++) {
        j["sheet"].push_back(sheet[i]->toJson());
    }
    return j;
}

void Database::fromJson(json j) {
    strcpy(name, j["name"].get<std::string>().c_str());
    sheet_num = j["sheet_num"].get<int>();
    mem = j["mem"].get<uint64_t>();
    for (uint i = 0; i < sheet_num; i++) {
        sheet[i] = new Sheet(this, j["sheet"][i]);
    }
}

void Database::update() {
    std::string j = toJson().dump(4);
    writeFile(dirPath(name, "database.udb"), j.c_str(), j.length());
}

Database::Database(const char* name, bool create) {
    fm = new FileManager();
    bpm = new BufPageManager(fm);
    if (create) {
        strcpy(this->name, name); 
        memset(sheet, 0, sizeof(sheet));
        sheet_num = 0;
        mem = 0;
        if (access(this->name, 0) == -1) {
            #ifdef WIN32
            mkdir(this->name);
            #elif __linux__
            mkdir(this->name, 0777);
            #endif
        }
        update();
        fm->createFile(dirPath(name, ".storage"));
    } else {
        fromJson(json::parse(readFile(dirPath(name, "database.udb"))));
    }
    fm->openFile(dirPath(name, ".storage"), mem_file);
}

Database::~Database() {
    update();
    fm->closeFile(mem_file);
    for (int i = 0; i < MAX_SHEET_NUM; i++) if (sheet[i] != nullptr) {
        delete sheet[i];
    }
    delete fm;
    delete bpm;
}

int Database::deleteSheet(const char* name){
    int num = this->findSheet(std::string(name));
    if(num == -1)return -1;
    sheet[num] = sheet[sheet_num];
    sheet[sheet_num --] = nullptr;
    return fm->deleteFile(dirPath(this->name, name, ".usid"));
}

Sheet* Database::createSheet(const char* name, int col_num, Type* col_ty) {
    Sheet* new_sheet = new Sheet();
    if (new_sheet->createSheet(sheet_num, this, name, col_num, col_ty, true)) {
        delete new_sheet;
        return nullptr;
    }
    sheet[sheet_num++] = new_sheet;
    update();
    return new_sheet;
}

Sheet* Database::openSheet(const char* name) {
    for (uint i = 0; i < sheet_num; i++) {
        if (strcmp(sheet[i]->name, name) == 0) {
            return sheet[i];
        }
    }
    return nullptr;
}

void Database::showSheets() {
    std::vector<std::pair<std::string, int> > v;
    v.push_back(std::pair<std::string, int>("Table", 20));
    Print::title(v);
    Anys d;
    for (uint i = 0; i < sheet_num; i++) {
        d.push_back(Any((char*)sheet[i]->name));
        Print::row(d);
        d.clear();
    }
    Print::end();
}
    
int Database::findSheet(std::string s) {
    for (uint i = 0; i < sheet_num; i++) if (std::string(sheet[i]->name) == s) return i;
    return -1;
}

Sheet* Database::findSheetPointer(std::string s) {
    int idx = findSheet(s);
    if (idx < 0) return nullptr; else return sheet[idx];
}

char* Database::getVarchar(uint64_t idx) {
    int index; 
    int size = (idx & 0xffff);
    BufType buf = bpm->getPage(mem_file, idx >> 32, index) + ((idx & 0xffffffff) >> 16);
    char* str = new char[size + 1];
    memcpy(str, buf, size);
    str[size] = '\0';
    return str;
}

uint64_t Database::storeVarchar(char* str) {
    uint size = strlen(str);
    uint64_t out = (mem << 16) + size;

    int index;
    uint page = mem >> 16;
    uint offset = mem & 0xffff;
    BufType buf = bpm->getPage(mem_file, page, index) + offset;
    while (true) {
        if (PAGE_SIZE - offset > size) {
            memcpy(buf, str, size * sizeof(char));
            offset += size;
            break;
        } else {
            memcpy(buf, str, (PAGE_SIZE - offset) * sizeof(char));
            str += PAGE_SIZE - offset;
            size -= PAGE_SIZE - offset;
            page += 1;
            offset = 0;
            bpm->markDirty(index);
            buf = bpm->getPage(mem_file, page, index) + offset;
        }
    }
    bpm->markDirty(index);
    mem = ((uint64_t)page << 16) + offset;
    return out;
}

bool Database::cmpCol(enumOp op, Anys a, Anys b) {
    switch (op) {
    case OP_EQ:
        return a == b;
    case OP_NEQ:
        return a != b;
    case OP_LE:
        return a <= b;
    case OP_GE:
        return a >= b;
    case OP_LS:
        return a < b;
    case OP_GT:
        return a > b;
    default:
        return false; // TODO
    }
}

bool Database::checkWhere(WhereStmt w) {
    Anys data, _data;
    for (auto it: w.cols) {
        data.push_back(filterSheet(it.first)->getPointerColData(it.second));
    }
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
            _data.push_back(filterSheet(it.first)->getPointerColData(it.second));
        }
        return cmpCol(w.op, data, _data);
    }
    // case 3:
    //     return false; // TODO
    //     break;
    default:
        return false;
    }
}

void Database::storeData(uint idx) {
    Anys data;
    if (sel[idx].select.size() == 0) {
        for (auto it: sel[idx].from) {
            data.concatenate(filterSheet(it.first)->getPointerData());
        }
    } else {
        for (auto it: sel[idx].select) {
            data.push_back(filterSheet(it.first)->getPointerColData(it.second));
        }
    }
    sel_sheet[idx]->data.push_back(data);
    sel_sheet[idx]->record_num += 1;
}

void Database::dfsCross(uint idx, uint f_idx) {
    if (f_idx == sel[idx].from.size()) {
        for (auto it: sel[idx].where) {
            if (checkWhere(it) == false) return;
        }
        storeData(idx);
    } else {
        filterSheet(sel[idx].from[f_idx].first)->setPointer(-1);
        while (filterSheet(sel[idx].from[f_idx].first)->movePointer()) {
            dfsCross(idx, f_idx + 1);
        }
    }
}

void Database::buildSel(uint idx) {
    for (auto it: sel[idx].recursion) buildSel(-1-it);
    dfsCross(idx, 0);
}