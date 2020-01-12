#include "Database.h"

#include "Table.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include "Print.h"

#include <vector>
#include <string>
#include <dirent.h>
#include <fcntl.h>
#include <termio.h>

#ifdef WIN32
#include <direct.h>
#elif __linux__
#include <sys/types.h>
#include <sys/stat.h>
#endif

#define filterTable(it) ((it) < 0 ? sel_table[-1-it] : table[it])

extern int cleanFiles(const char *dir);
extern char* readFile(const char* path);
extern void writeFile(const char* path, const char* data, const int length);
extern char* dirPath(const char* dir);
extern char* dirPath(const char* dir, const char* path);
extern char* dirPath(const char* dir, const char* filename, const char* filetype);
extern int checkFile(const char *filename);

inline char getch() {
    struct termios tm, tm_old;
    int fd = 0, ch;
    if (tcgetattr(fd, &tm) < 0) return -1;
    tm_old = tm;
    cfmakeraw(&tm);
    if (tcsetattr(fd, TCSANOW, &tm) < 0) return -1;
    ch = getchar();
    if (tcsetattr(fd, TCSANOW, &tm_old) < 0) return -1;
    return ch;
}

json Database::toJson() {
    json j;
    j["name"] = name;
    j["table_num"] = table_num;
    j["mem"] = mem;
    for (uint i = 0; i < table_num; i++) {
        j["table"].push_back(table[i]->toJson());
    }
    return j;
}

void Database::fromJson(json j) {
    strcpy(name, j["name"].get<std::string>().c_str());
    table_num = j["table_num"].get<int>();
    mem = j["mem"].get<uint64_t>();
    for (uint i = 0; i < table_num; i++) {
        table[i] = new Table(this, j["table"][i]);
    }
}

void Database::update() {
    std::string j = toJson().dump(4);
    writeFile(dirPath(name, "database.udb"), j.c_str(), j.length());
}

Database::Database(const char* name, bool create) {
    fm = new FileManager();
    bpm = new BufPageManager(fm);
    memset(table, 0, sizeof(table));
    table_num = 0;
    mem = 0;
    if (create) {
        strcpy(this->name, name); 
        char* dbdir = dirPath(this->name);
        if (access("db", 0) == -1) {
            #ifdef WIN32
            mkdir("db");
            #elif __linux__
            mkdir("db", 0777);
            #endif
        }
        if (access(dbdir, 0) == -1) {
            #ifdef WIN32
            mkdir(dbdir);
            #elif __linux__
            mkdir(dbdir, 0777);
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
    for (int i = 0; i < MAX_TABLE_NUM; i++) if (table[i] != nullptr) {
        delete table[i];
    }
    delete fm;
    delete bpm;
}

Table* Database::createTable(const char* name, int col_num, Type* col_ty) {
    Table* new_table = new Table();
    if (new_table->createTable(table_num, this, name, col_num, col_ty, true)) {
        delete new_table;
        return nullptr;
    }
    table[table_num++] = new_table;
    update();
    return new_table;
}

Table* Database::openTable(const char* name) {
    for (uint i = 0; i < table_num; i++) {
        if (strcmp(table[i]->name, name) == 0) {
            return table[i];
        }
    }
    return nullptr;
}

int Database::deleteTable(const char* name) { // -1: doesn't exist, 0: OK
    int num = this->findTable(std::string(name));
    if (num == -1) return -1;
    delete table[num];
    for (uint i = num; i < table_num - 1; i++) {
        table[i] = table[i + 1];
        table[i]->table_id = i;
    }
    table[--table_num] = nullptr;
    fm->deleteFile(dirPath(this->name, name, "usid"));
    update();
    return 0;
}

void Database::showTables() {
    std::vector<std::pair<std::string, int> > v;
    v.push_back(std::pair<std::string, int>("Table", 20));
    Print::title(v);
    Anys d;
    for (uint i = 0; i < table_num; i++) {
        d.push_back(Any((char*)table[i]->name));
        Print::row(d);
        d.clear();
    }
    Print::end();
}
    
int Database::findTable(std::string s) { // -1: doesn't exist, others: OK
    for (uint i = 0; i < table_num; i++) if (std::string(table[i]->name) == s) return i;
    return -1;
}

char* Database::getVarchar(uint64_t idx) {
    uint page = idx >> 32;
    uint offset = (idx & 0xffffffff) >> 16;
    uint size = idx & 0xffff;
    BufType buf = bpm->getPage(mem_file, page) + offset;
    char* str = new char[size + 1]; 
    char* _str = str;
    while (true) {
        if (PAGE_SIZE - offset > size) {
            memcpy(str, buf, size);
            offset += size;
            break;
        } else {
            memcpy(str, buf, (PAGE_SIZE - offset));
            str += PAGE_SIZE - offset;
            size -= PAGE_SIZE - offset;
            page += 1;
            offset = 0;
            buf = bpm->getPage(mem_file, page) + offset;
        }
    }
    
    str[size] = '\0';
    return _str;
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
            memcpy(buf, str, size);
            offset += size;
            break;
        } else {
            memcpy(buf, str, (PAGE_SIZE - offset));
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

bool cmpCol(enumOp op, Anys &a, Anys &b) {
    switch (op) {
    case OP_EQ: return a == b;
    case OP_NEQ: return a != b;
    case OP_LE: return a <= b;
    case OP_GE: return a >= b;
    case OP_LS: return a < b;
    case OP_GT: return a > b;
    default: return false;
    }
}

bool Database::checkWhere(WhereStmt &w) { // Table::checkWhere
    Anys data, _data;
    for (auto it: w.cols) {
        if (filterTable(it.first)->checkPointer() == false) return true;
        data.push_back(filterTable(it.first)->getPointerColData(it.second));
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
            if (filterTable(it.first)->checkPointer() == false) return true;
            _data.push_back(filterTable(it.first)->getPointerColData(it.second));
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

void Database::storeData(uint idx) {
    Anys data;
    if (sel[idx].select.size() == 0) {
        if (sel[idx].aggr.size() == 0) {
            for (auto it: sel[idx].from) {
                data.concatenate(filterTable(it.first)->getPointerData());
            }
        } else {
            for (auto it: sel[idx].aggr) if (it.ty != AG_COUNT) {
                data.push_back(filterTable(it.col.first)->getPointerColData(it.col.second));
            }
        }
    } else {
        for (auto it: sel[idx].select) {
            data.push_back(filterTable(it.first)->getPointerColData(it.second));
        }
    }
    if (data.size()) sel_table[idx]->data.push_back(data);
    sel_table[idx]->record_num += 1;
}

bool selStop;

void Database::dfsCross(uint idx, uint f_idx, bool print = false) {
    for (auto it: sel[idx].where) {
        if (checkWhere(it) == false) return;
    }
    if (f_idx == sel[idx].from.size()) {
        storeData(idx);
        if (print && sel_table[idx]->record_num % MAX_RESULT_LINES == 0 && sel_table[idx]->printBack(MAX_RESULT_LINES)) {
            puts("====== Press c to continue, q to quit ======");
            while (true) {
                char ch = getch();
                if (ch == 'c') {
                    puts("================= Continue =================");
                    break;
                }
                if (ch == 'q') { 
                    selStop = true;
                    puts("=================== Quit ===================");
                    return; 
                }
            }
        }
    } else {
        filterTable(sel[idx].from[f_idx].first)->initPointer();
        while (filterTable(sel[idx].from[f_idx].first)->movePointer()) {
            dfsCross(idx, f_idx + 1, print);
            if (selStop) {
                filterTable(sel[idx].from[f_idx].first)->removePointer();
                return;
            }
        }
        filterTable(sel[idx].from[f_idx].first)->removePointer();
    }
}

void Database::buildSel(uint idx, bool print) {
    if (sel[idx].build) return; 
    sel[idx].build = true;
    for (auto it: sel[idx].recursion) buildSel(-1-it);
    selStop = false;
    for (int i = 0; i < sel[idx].from.size(); i++) 
        filterTable(sel[idx].from[i].first)->removePointer();
    dfsCross(idx, 0, print);
    if (sel[idx].select.size() == 0 && sel[idx].aggr.size() > 0) {
        uint _idx = 0;
        Table* table = sel_table[idx];
        for (auto it: sel[idx].aggr) switch (it.ty) {
            case AG_COUNT: {
                table->val.push_back(Any((int)table->record_num));
                break;
            }
            case AG_MIN: {
                Any min = Any();
                for (auto _it: table->data) if (min.isNull()) min = _it[_idx]; else if (_it[_idx].isNull()) continue; else if (_it[_idx] < min) min = _it[_idx];
                table->val.push_back(min);
                _idx++;
                break;
            }
            case AG_MAX: {
                Any max = Any();
                for (auto _it: table->data) if (max.isNull()) max = _it[_idx]; else if (_it[_idx].isNull()) continue; else if (_it[_idx] > max) max = _it[_idx];
                table->val.push_back(max);
                _idx++;
                break;
            }
            case AG_SUM: 
            case AG_AVG: {
                if (table->col_ty[_idx].ty == INT) {
                    int sum = 0;
                    int num = 0;
                    for (auto _it: table->data) if (_it[_idx].isNull() == false) num++, sum += _it[_idx].anyRefCast<int>();
                    if (it.ty == AG_SUM) table->val.push_back(Any(sum)); else table->val.push_back(Any((long double)sum / num));
                } else {
                    long double sum = 0;
                    int num = 0;
                    for (auto _it: table->data) if (_it[_idx].isNull() == false) num++, sum += _it[_idx].anyRefCast<long double>();
                    if (it.ty == AG_SUM) table->val.push_back(Any(sum)); else table->val.push_back(Any(sum / num));
                }
                _idx++;
                break;
            }
        }
    }
    uint _idx = 0;
    for (auto it: sel[idx].aggr) {
        if (it.ty == AG_AVG) sel_table[idx]->col_ty[_idx].ty = DECIMAL;
        if (it.ty != AG_COUNT) _idx++;
    }
    if (print && selStop == false && sel_table[idx]->record_num % MAX_RESULT_LINES && sel_table[idx]->printBack(sel_table[idx]->record_num % MAX_RESULT_LINES) == false) {
        sel_table[idx]->print();
    }
}

void showDatabases() {
    std::vector<std::pair<std::string, int> > v;
    v.push_back(std::pair<std::string, int>("Database", 20));
    Print::title(v);
    Anys d;

    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    
    stat("db", &dir_stat);
    if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir("db");
        while ((dp=readdir(dirp)) != NULL) {
            if ((0 == strcmp(".", dp->d_name)) || (0 == strcmp("..", dp->d_name))) { // Ignore . & ..
                continue;
            }
            stat(dp->d_name, &dir_stat);
            if (S_ISDIR(dir_stat.st_mode)) {
                if (checkFile(dirPath(dp->d_name, ".storage")) == 1) {
                    d.push_back(Any((char*)dp->d_name));
                    Print::row(d);
                    d.clear();
                }
            }
        }
        closedir(dirp);
    }
    
    Print::end();
}

int cleanDatabase(const char *dbname) {
    return cleanFiles(dirPath(dbname));
}

