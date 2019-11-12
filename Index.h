#ifndef __INDEX
#define __INDEX

#include "constants.h"

#include "json.hpp"
using json = nlohmann::json;

class BufPageManager;
class Sheet;

class Index {
public:
    uint32_t key_cal; // Tree, Pointer, TODO: Finally will be index_cal
    bool main; // main index or not
    int value_index; // if not main, which value will be choosed
    uint32_t page_num;
    uint32_t root_page;
    uint32_t next_del_page;
    uint16_t leaf_record_size;
    uint16_t nonleaf_record_size;
    int fileID;
    BufPageManager* bpm;
    Sheet* sheet;

private:
    // IndexRecord* BTreeInsert(uint32_t pageID, Any key, Record* record);
    // Any calKeyValue(Record* record, uint32_t key_cal);

public:
    // Index();
    // json toJson();
};

#endif