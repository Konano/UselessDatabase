#ifndef __INDEX
#define __INDEX

#include "constants.h"
#include "Any.h"
#include "Anys.h"
#include "Type.h"
#include "Database.h"

#include <vector>

#include "json.hpp"
using json = nlohmann::json;

class BtreeNode;
class BufPageManager;
class Table;

class Index {
public:
    Table* table;
    char name[MAX_NAME_LEN];
    uint key_num;
    std::vector<uint> key;
    std::vector<uint> offset;
    uint page_num;
    uint root_page;
    uint next_del_page;
    uint16_t record_size;
    uint max_recond_num;
    int fileID;

    std::vector<enumType> ty;

    int btree_max_per_node;
    int btree_root_index;
private:

    void overflow_upstream(int now);
    void overflow_downstream(int now);
    std::vector<int> queryRecord(Anys* info, int now);
    void insertRecord(Anys* info, int record_id, int now);
    void removeRecord(Anys* info, int record_id, int now);
    uint queryRecordsNum(enumOp op, Anys& data, int now);
    std::vector<uint> queryRecords(enumOp op, Anys& data, int now);

public:
    Index() {}
    Index(Table* table, const char* name, std::vector<uint> key, int btree_max_per_node);
    Index(Table* table, json j);
    json toJson();

    void open();
    void close();
    void remove();

    BtreeNode* convert_buf_to_BtreeNode(int index);
    void convert_BtreeNode_to_buf(BtreeNode* node);
    void Btree_remove(BtreeNode* node);

    std::vector<int> queryRecord(Anys* info);
    void insertRecord(Anys* info, int record_id);
    void removeRecord(Anys* info, int record_id);

    void Debug();
    void debug(int now);

    uint queryRecordsNum(enumOp op, Anys& data);
    std::vector<uint> queryRecords(enumOp op, Anys& data);
};

#endif