#ifndef __SHEET
#define __SHEET

#include "constants.h"
#include "Type.h"
#include "Index.h"
#include "Key.h"
#include "Anys.h"
#include "Database.h" // enumOp

#include "json.hpp"
using json = nlohmann::json;

class Database;
class FileManager;
class BufPageManager;

class Sheet {
private:
    void insert(Any& val, enumType ty, uint size, BufType& buf);
    void fetch(BufType& buf, enumType ty, uint size, Any& val);
    void fetchWithOffset(BufType& buf, enumType ty, uint size, Any& val, uint offset);
    uint genOffset(uint index);
    char* getStr(BufType buf, uint size);
    
public:
    char name[MAX_NAME_LEN];
    // char comment[MAX_COMMENT_LEN];
    Database* db;
    FileManager* fm;
    BufPageManager* bpm;
    uint sheet_id;
    uint col_num = 0; // TODO Drop this, change to vector
    Type col_ty[MAX_COL_NUM];
    PrimaryKey* p_key = nullptr; // primary key
    std::vector<ForeignKey*> f_key; // foreign keys
    int p_key_index = -1;
    uint index_num = 0;
    Index index[MAX_INDEX_NUM];
    uint record_num = 0; // all record, include removed record
    int main_file;
    uint record_size;
    uint record_onepg;
    
    uint sel = 0;
    std::vector<Anys> data; // when sel = 1
    std::vector<Any> val;   // when sel = 2

    json toJson();
    Sheet(Database* db, json j);

    Sheet() {}
    Sheet(uint _sel) : sel(_sel) {}
    ~Sheet();
    uint calDataSize();
    int createSheet(uint sheet_id,Database* db, const char* name, uint col_num, Type* col_ty, bool create = false);
    int insertRecord(Any* data);
    // TODO void removeRecord(const int len, Any* info);
    int removeRecord(const int record_id);
    Anys queryRecord(const int record_id);
    int queryRecord(const int record_id, Any* &data);
    int updateRecord(const int record_id, const int len, Any* data);

    bool cmpRecords(Anys data, enumOp op, bool any, bool all);

    int findIndex(std::string s);
    uint createIndex(std::vector<uint> key_index,std::string name);
    uint createKeyIndex(Key* key); // TODO
    void removeIndex(uint index_id);

    int createColumn(Type ty);
    int removeColumn(uint key_index);
    int modifyColumn(uint key_index, Type ty);
    void updateColumns();

    void rebuild(int ty, uint key_index);

    int createForeignKey(ForeignKey* fk, PrimaryKey* pk);
    int removeForeignKey(ForeignKey* fk);
    int createPrimaryKey(PrimaryKey* pk);
    int removePrimaryKey();
    // bool queryPrimaryKey(Any* info); 

    void printCol();
    void print();

    int findCol(std::string a);
    int constraintCol(uint col_id);
    int constraintKey(Key* key);
    int constraintRow(Any* data, uint record_id, bool ck_unique);
    int constraintRowKey(Any* data, Key* key);

    int pointer;
    void setPointer(int pointer);
    bool movePointer();
    Any getPointerColData(uint idx);
    Anys getPointerData();
};

#endif

// TODO i -> record_id

// TODO key_index -> col_id

// TODO 名字比较需要无视大小写