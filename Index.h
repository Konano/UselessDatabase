#ifndef __INDEX
#define __INDEX

#include "constants.h"

#include "json.hpp"
using json = nlohmann::json;

class BufPageManager;
class Sheet;

class Index {
public:
    Sheet* sheet;
    char name[MAX_NAME_LEN];
    uint key_cal; // Tree, Pointer, TODO: Finally will be index_cal
    uint page_num;
    uint root_page;
    uint next_del_page;
    uint16_t record_size;
    int fileID;

private:
    // IndexRecord* BTreeInsert(uint32_t pageID, Any key, Record* record);
    // Any calKeyValue(Record* record, uint32_t key_cal);

public:
    Index() {}
    Index(Sheet* sheet, const char* name, uint key_cal);
    Index(Sheet* sheet, json j);
    json toJson();
};

#endif