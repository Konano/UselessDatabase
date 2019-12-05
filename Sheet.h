#ifndef __SHEET
#define __SHEET

#include "constants.h"
#include "Type.h"
#include "Index.h"

#include "json.hpp"
using json = nlohmann::json;

class Database;
class FileManager;
class BufPageManager;

class Sheet {
public:
    char name[MAX_NAME_LEN];
    // char comment[MAX_COMMENT_LEN];
    Database* db;
    FileManager* fm;
    BufPageManager* bpm;
    uint sheet_id;
    uint col_num = 0;
    Type col_ty[MAX_COL_NUM];
    int pri_key = -1;
    int pri_key_index = -1;
    uint index_num = 0;
    Index index[MAX_INDEX_NUM];
    uint record_num = 0; // all record, include removed record
    int main_file;
    uint record_size;
    uint record_onepg;

    json toJson();
    Sheet(Database* db, json j);

    Sheet() {}
    uint calDataSize();
    int createSheet(uint sheet_id,Database* db, const char* name, int col_num, Type* col_ty, bool create = false);
    int insertRecord(Any* info);
    // void removeRecord(const int len, Any* info);
    int removeRecord(const int record_id);
    int queryRecord(const int record_id, Any* &info);
    int updateRecord(const int record_id, const int len, Any* info);

    void createIndex(uint key_index);
    void removeIndex(uint index_id);

    void createColumn(Type ty);
    void removeColumn(uint key_index);
    void modifyColumn(uint key_index, Type ty);

    void rebuild(int ty, uint key_index);
    int createForeignKey(uint key_index, uint sheet_id);
    int removeForeignKey(uint key_index);
    int createPrimaryKey(uint key_index);
    int removePrimaryKey(uint key_index);
    bool queryPrimaryKey(Any query_val); 

    void printCol();
    void print();
};

#endif