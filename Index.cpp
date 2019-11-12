#include "Index.h"

#include "BufPageManager.h"
#include "Sheet.h"
#include "FileManager.h"
#include "Database.h"

extern char* Dir(const char* dir, const char* filename, const char* name, const char* suffix);

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

// uint32_t key_cal; // Tree, Pointer, TODO: Finally will be index_cal
//     uint32_t page_num;
//     uint32_t root_page;
//     uint32_t next_del_page;
//     uint16_t leaf_record_size;
//     uint16_t nonleaf_record_size;
//     int fileID;
//     BufPageManager* bpm;
//     Sheet* sheet;

json Index::toJson() {
    json j;
    j["name"] = name;
    j["key_cal"] = key_cal;
    j["page_num"] = page_num;
    j["root_page"] = root_page;
    j["next_del_page"] = next_del_page;
    j["record_size"] = record_size;
    return j;
}

Index::Index(Sheet* sheet, const char* name, uint key_cal) : sheet(sheet), key_cal(key_cal) {
    strcpy(this->name, name);
    sheet->fm->createFile(Dir(sheet->db->name, sheet->name, name, ".usid"));
    sheet->fm->openFile(Dir(sheet->db->name, sheet->name, name, ".usid"), fileID);
    page_num = root_page = next_del_page = 0;
    record_size = sheet->col_ty[key_cal].size() + 2 + 4;
}

Index::Index(Sheet* sheet, json j) : sheet(sheet) {
    strcpy(name, j["name"].get<std::string>().c_str());
    key_cal = j["key_cal"].get<int>();
    page_num = j["page_num"].get<int>();
    root_page = j["root_page"].get<int>();
    next_del_page = j["next_del_page"].get<int>();
    record_size = j["record_size"].get<int>();
    sheet->fm->openFile(Dir(sheet->db->name, sheet->name, name, ".usid"), fileID);
}

