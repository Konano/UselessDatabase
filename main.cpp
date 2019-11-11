// TODO: makefile
// TODO: all define should be in a file

#include "constants.h"
#include "Any.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include "Database.h"
#include "Sheet.h"
#include "Type.h"


struct IndexRecord {
    uint16_t NEXT_DEL_RECORD;
    uint RECORD_ID;
    uint8_t KEY; // TODO: its length is not constant
};

/*
class Page {
public:
    BufType buf;
    uint32_t left_page;
    uint32_t right_page;
    uint32_t next_del_page;
    uint16_t next_del_record;
    uint16_t next_unuse_place;
    uint16_t record_count;
    bool leaf_or_not;
    // uint32_t RECORDS;
    // uint32_t SORTED_QUEUE;

    Page(BufType _buf) : buf(_buf) {
        left_page = ((uint32_t*)(buf))[0];
        right_page = ((uint32_t*)(buf))[1];
        next_del_page = ((uint32_t*)(buf))[2];
        next_del_record = ((uint32_t*)(buf))[3];
        next_unuse_place = ((uint32_t*)(buf))[4];
        record_count = ((uint32_t*)(buf))[5] >> 1;
        leaf_or_not = ((uint32_t*)(buf))[5] & 1;
    }

    BufType* assemble(BufType buf) {
        ((uint32_t*)(buf))[0] = left_page;
        ((uint32_t*)(buf))[1] = right_page;
        ((uint32_t*)(buf))[2] = next_del_page;
        ((uint32_t*)(buf))[3] = next_del_record;
        ((uint32_t*)(buf))[4] = next_unuse_place;
        ((uint32_t*)(buf))[5] = (record_count << 1) | leaf_or_not;
    }
};
*/



struct Record {
    int recordID;
    Any* data;
};

struct ForeignKey {
    // TODO:
};

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

// TODO: 修改一个 Key 后可能要修改外键 & Index
// TODO: VARCHAR




int main() {
    // Init
    Database *db = new Database("TestDatabase", true); // New Database
    Sheet *sheet = db->createSheet("TestSheet", 4, new Type[4]{Type("Number"), Type("Name", enumType::CHAR, 3), Type("Height"), Type("Weigh")});
    sheet->insertRecord(4, new Any[4]{2017011474, (char*)"ZLK", 160, 80});
    // sheet->removeRecord(0);
    // sheet->updateRecord();
    // sheet->queryRecord();
    
    // Open a database
    // Create a sheet
    // Add a record
    delete db;
}