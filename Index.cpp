#include "Index.h"

#include "BufPageManager.h"
#include "Sheet.h"

/*
struct Index {
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

private:
    IndexRecord* BTreeInsert(uint32_t pageID, Any key, Record* record) {
        if (page_num == 0) {
            // TODO:
            // BufType buf = bpm->getPage(fileID, page_num++);
            // Page page = Page::newPage(buf, record);
            // page.assemble(buf);
        } else {
            Page page = Page(bpm->getPage(fileID, pageID));
            if (page.leaf_or_not == false) {
                int lb = find(pageID, record);
                IndexRecord* a = BTreeInsert(page.record[lb], key, record);
                if (a != nullptr) {
                    if (page.full()) {
                        pageflip();
                    } else { 
                        page.insert(lb, a);
                        return nullptr;
                    }
                }
                return nullptr;
            } else {
                int lb = find(pageID, record);
                page.insert(lb, LeafRecord(record));
                return nullptr;
            }
        }
    }

    Any calKeyValue(Record* record, uint32_t key_cal);

public:

    Index() {
        // fm->openFile
    }

    // void insertRecord(const int len, Any* info) {
    //     if (main) {
    //         BTreeInsert(root_page, NULL, record);
    //     } else {
    //         BTreeInsert(root_page, calKeyValue(record, key_cal), record);
    //     }
    // }
};
*/