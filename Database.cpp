#include "Database.h"

#include "Sheet.h"
#include "FileManager.h"
#include "BufPageManager.h"

#ifdef WIN32
#include <direct.h>
#elif __linux__
#include <sys/types.h>
#include <sys/stat.h>
#endif

extern char* readFile(const char* path);
extern void writeFile(const char* path, const char* data, const int length);
extern char* dirPath(const char* dir, const char* path);

json Database::toJson() {
    json j;
    j["name"] = name;
    j["sheet_num"] = sheet_num;
    for (int i = 0; i < sheet_num; i++) {
        j["sheet"].push_back(sheet[i]->toJson());
    }
    return j;
}

void Database::fromJson(json j) {
    strcpy(name, j["name"].get<std::string>().c_str());
    sheet_num = j["sheet_num"].get<int>();
    for (int i = 0; i < sheet_num; i++) {
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
        if (access(this->name, 0) == -1) {
            #ifdef WIN32
            mkdir(this->name);
            #elif __linux__
            mkdir(this->name, 0777);
            #endif
        }
        update();
    } else {
        fromJson(json::parse(readFile(dirPath(name, "database.udb"))));
    }
}

Database::~Database() {
    update();
    delete fm;
    delete bpm;
    for (int i = 0; i < MAX_SHEET_NUM; i++) if (sheet[i] != nullptr) {
        delete sheet[i];
    }
}

Sheet* Database::createSheet(const char* name, int col_num, Type* col_ty) {
    Sheet* new_sheet = new Sheet(this, name, col_num, col_ty, true);
    sheet[sheet_num++] = new_sheet;
    update();
    return new_sheet;
}

Sheet* Database::openSheet(const char* name) {
    for (int i = 0; i < sheet_num; i++) {
        if (strcmp(sheet[i]->name, name) == 0) {
            return sheet[i];
        }
    }
    return nullptr;
}
