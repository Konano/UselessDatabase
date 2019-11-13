#ifndef __INDEX
#define __INDEX

#include "constants.h"
#include "Any.h"

#include "json.hpp"
using json = nlohmann::json;

class BufPageManager;
class Sheet;

class Index {
public:
    Sheet* sheet;
    char name[MAX_NAME_LEN];
    uint key; // Tree, Pointer, TODO: Finally will be index_cal
    uint page_num;
    uint root_page;
    uint next_del_page;
    uint16_t record_size;
    uint rank;
    //node *root;
    int fileID;

private:
    // IndexRecord* BTreeInsert(uint32_t pageID, Any key, Record* record);
    // Any calKeyValue(Record* record, uint32_t key);
    void initIndex();


public:
    Index() {}
    Index(Sheet* sheet, const char* name, uint key);
    Index(Sheet* sheet, json j);
    json toJson();

    void open();
    void close();

    int queryRecord(const int len, Any* info);

    void insertRecord(const int len, Any* info);
    void removeRecord(const int len, Any* info);
    int searchRecord(const int len, Any* info);
};

#endif