#ifndef __INDEX
#define __INDEX

#include "constants.h"
#include "Any.h"
#include "Anys.h"
#include "Type.h"
#include "BtreeNode.h"

#include <vector>
using namespace std;

#include "json.hpp"
using json = nlohmann::json;

class BufPageManager;
class Sheet;

class Index {
public:
    Sheet* sheet;
    char name[MAX_NAME_LEN];
    uint key_num;
    vector<uint> key;
    vector<uint> offset;
    uint page_num;
    uint root_page;
    uint next_del_page;
    uint16_t record_size;
    uint max_recond_num;
    BtreeNode* root;
    int fileID;

    vector<enumType> ty;

    int btree_max_per_node;
    int btree_root_index;
private:
    // IndexRecord* BTreeInsert(uint32_t pageID, Any key, Record* record);
    // Any calKeyValue(Record* record, uint32_t key);
    // void initIndex();

    void overflow_upstream(BtreeNode* now);
    void overflow_downstream(BtreeNode* now);
    vector<int> queryRecord(Anys* info, int index);
    void insertRecord(Anys* info, int record_id, BtreeNode* now);
    void removeRecord(Anys* info, int record_id, BtreeNode* now);

public:
    Index() {}
    Index(Sheet* sheet, const char* name, vector<uint> key,int btree_max_per_node);
    Index(Sheet* sheet, json j);
    json toJson();

    void open();
    void close();
    void remove();

    BtreeNode* convert_buf_to_BtreeNode(int index);
    void convert_BtreeNode_to_buf(BtreeNode* node);
    void Btree_remove(BtreeNode* node);

    vector<int> queryRecord(Anys* info);
    void insertRecord(Anys* info, int record_id);
    void removeRecord(Anys* info, int record_id);

    void Debug();
    void debug(BtreeNode* node);
};

#endif