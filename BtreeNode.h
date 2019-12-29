#ifndef __BTREENODE
#define __BTREENODE

#include "constants.h"
#include "Anys.h"

struct BtreeRecord{
    int record_id; // RECORD_ID
    Anys key; // KEY
    BtreeRecord() {}
    BtreeRecord(int record_id, Anys key) : record_id(record_id) , key(key) {}
};

struct BtreeNode{
    int left_page_index; // LEFT_PAGE
    int right_page_index; // RIGHT_PAGE 
    BtreeNode *left_page; // RIGHT_PAGE_PTR
    BtreeNode *right_page; // LEFT_PAGE_PTR
    int index; // PAGE_INDEX
    int record_cnt; // RECORD_COUNT
    std::vector<int> child; // CHILD_INDEX
    std::vector<BtreeNode*> child_ptr; // CHILD_NODE
    std::vector<BtreeRecord> record; // RECORDS
    bool is_leaf;

    BtreeNode() {
        this->is_leaf = true;
        this->fa_index = -1;
        this->record_cnt = 0;
        this->record.clear();
        this->child.clear();
        this->child.push_back(-1);
    };

    int fa_index; // FATHER_INDEX
};

#endif