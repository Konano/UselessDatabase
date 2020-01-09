#ifndef __TABLE
#define __TABLE

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

class Table {
private:
    void insert(Any& val, enumType ty, uint size, BufType& buf);
    void fetch(BufType& buf, enumType ty, uint size, Any& val);
    void fetchWithOffset(BufType& buf, enumType ty, uint size, Any& val, uint offset);
    uint genOffset(uint index);
    char* getStr(BufType buf, uint size);
    
    std::vector<uint> existentRecords();
    bool checkWhere(Anys data, WhereStmt &w);
    bool checkWheres(Anys data, std::vector<WhereStmt> &where);
    std::vector<uint> findWheres(std::vector<WhereStmt> &where);
    Anys chooseData(Anys& data, std::vector<uint>& cols);

    struct Pointer {
        Table* s;
        uint pid;
        BufType buf;
        void init();
        bool next();
        Pointer() {}
        Pointer(Table* _s, uint _pid);
        Anys get();
    } pointer;
    
public:
    char name[MAX_NAME_LEN];
    // char comment[MAX_COMMENT_LEN];
    Database* db;
    FileManager* fm;
    BufPageManager* bpm;
    uint table_id;
    uint col_num = 0;
    Type col_ty[MAX_COL_NUM];
    PrimaryKey* p_key = nullptr; // primary key
    std::vector<ForeignKey*> f_key; // foreign keys
    int p_key_index = -1;
    std::vector<int> f_key_index;
    uint index_num = 0;
    Index index[MAX_INDEX_NUM];
    uint record_num = 0; // all record, include removed record
    int main_file;
    uint record_size;
    uint record_onepg;
    
    uint sel = 0;
    std::vector<Anys> data; // when sel = 1
    Anys val;   // when sel = 2

    json toJson();
    Table(Database* db, json j);

    Table() { this->pointer.s = this; }
    Table(uint _sel) : sel(_sel) { this->pointer.s = this; }
    ~Table();
    uint calDataSize();
    int createTable(uint table_id, Database* db, const char* name, uint col_num, Type* col_ty, bool create = false);
    int insertRecord(Any* data);
    // void removeRecord(const int len, Any* info);
    int removeRecord(const int record_id);
    Anys queryRecord(const int record_id);
    int queryRecord(const int record_id, Any* &data);
    // int updateRecord(const int record_id, const int len, Any* data);
    int updateRecord(const int record_id, std::vector<Pia> &set);

    int removeRecords(std::vector<WhereStmt> &where);
    int updateRecords(std::vector<Pia> &set, std::vector<WhereStmt> &where);

    bool cmpRecord(Anys a, Anys b, enumOp op);
    bool cmpRecords(Anys data, enumOp op, bool any, bool all);

    int findIndex(std::string s);
    uint createIndex(std::vector<uint> col_id, std::string name);
    // uint createKeyIndex(Key* key); 
    void removeIndex(uint index_id);

    int createColumn(Type ty);
    int removeColumn(uint col_id);
    int modifyColumn(uint col_id, Type ty);
    void updateColumns();

    void rebuild(int ty, uint col_id);

    int createForeignKey(ForeignKey* fk, PrimaryKey* pk);
    int removeForeignKey(ForeignKey* fk);
    int createPrimaryKey(PrimaryKey* pk);
    int removePrimaryKey();
    // bool queryPrimaryKey(Any* info); 

    void print();
    void printCol();
    bool printBack(uint num);

    int findCol(std::string a);
    int constraintCol(uint col_id);
    int constraintKey(Key* key);
    int constraintRow(Any* data, uint record_id, bool ck_unique);
    int constraintRowKey(Any* data, Key* key);

    void initPointer();
    bool movePointer();
    Any getPointerColData(uint idx);
    Anys getPointerData();
    uint getPointer();
};

#endif