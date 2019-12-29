#ifndef __BTREENODE
#define __BTREENODE

#include "constants.h"
#include "Anys.h"

struct BtreeRecord{
    //RECORD_ID
    int record_id;

    //KEY
    Anys key;
    BtreeRecord() {}
    BtreeRecord(int record_id, Anys key) : record_id(record_id) , key(key) {}
};

struct BtreeNode{
    //LEFT_PAGE
    int left_page_index;

    //RIGHT_PAGE
    int right_page_index;

    //RIGHT_PAGE_PTR
    BtreeNode *left_page;

    //LEFT_PAGE_PTR
    BtreeNode *right_page;

    //PAGE_INDEX
    int index;

    //RECORD_COUNT
    int record_cnt;

    //CHILD_INDEX
    std::vector<int> child;

    //CHILD_NODE
    std::vector<BtreeNode*> child_ptr;

    //RECORDS
    std::vector<BtreeRecord> record;


    bool is_leaf;

    BtreeNode() {
        this->is_leaf = true;
        this->fa_index = -1;
        this->record_cnt = 0;
        this->child.clear();
        this->child.push_back(-1);
    };

    //FATHER_INDEX
    int fa_index;

};

#endif