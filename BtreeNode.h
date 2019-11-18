#ifndef __BTREENODE
#define __BTREENODE

#include "constants.h"
#include "Any.h"

struct BtreeRecord{
    //RECORD_ID
    int record_id;

    //KEY
    Any key;
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

    //RECORD_SIZE
    int record_size;

    //CHILD_INDEX
    std::vector<int> child;

    //CHILD_NODE
    std::vector<BtreeNode*> child_ptr;

    //RECORDS
    std::vector<BtreeRecord> record;
};

#endif