#ifndef __SHEET
#define __SHEET

#include "constants.h"
#include "Database.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include "Sheet.h"
#include "Type.h"

#include "json.hpp"
using json = nlohmann::json;

class Database;

class Sheet {
public:
    char name[MAX_NAME_LEN];
    // char comment[MAX_COMMENT_LEN];
    Database* db;
    FileManager* fm;
    BufPageManager* bpm;
    uint col_num = 0;
    Type col_ty[MAX_COL_NUM];
    // ForeignKey f_key[MAX_COL_NUM];
    // int index_num = 0;
    // Index index[MAX_INDEX_NUM];
    uint record_num = 0;
    int main_file;
    uint record_size;
    uint record_onepg;

    // char* getFileName(int ty);
    uint calDataSize();
    Sheet(Database* db, const char* name, int col_num, Type* col_ty, bool create = false);
    void insertRecord(const int len, Any* info);
    void removeRecord(const int record_id);
    int quertRecord(const int record_id);

    json toJson();
    void fromJson(json j);
};

#endif