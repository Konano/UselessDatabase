// TODO: makefile

#include "Any.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Record {
    uint16_t NEXT_DEL_RECORD;
    uint32_t POINT_TO_PAGE;
    unsigned int LENGTH;
    uint8_t *INFO;
};

struct Page {
    uint32_t LEFT_PAGE;
    uint32_t RIGHT_PAGE;
    uint32_t NEXT_DEL_PAGE;
    uint16_t NEXT_DEL_RECORD;
    uint16_t NEXT_UNUSE_PLACE;
    uint16_t LEAF_OR_NOT_AND_RECORD_COUNT; // conbine
    // uint32_t RECORDS;
    // uint32_t SORTED_QUEUE;
};

#define MAX_NAME_LEN 32
#define MAX_COMMENT_LEN 256
#define MAX_SHEET_NUM 256
#define MAX_COL_NUM 32
#define MAX_INDEX_NUM 256

enum enumType {
    INT,
    FLOAT,
    CHAR,
    VARCHAR
};

enum enumKeyType {
    Primary,
    Foreign,
    Common
};

struct Type {
    char name[MAX_NAME_LEN];
    char comment[MAX_COMMENT_LEN];
    enumType ty = enumType::INT;
    uint8_t bytes_num = 0; // only CHAR will use it
    enumKeyType key = enumKeyType::Common;
    bool unique = false;
    bool null = true;
    Any def = nullptr; 
};

struct ForeignKey {
    // TODO:
};

struct Index {
    uint8_t index_key; // TODO: Finally will be index_cal
    bool main; // main index or not
    uint32_t page_num;
    uint32_t root_page;
    uint32_t next_del_page;
    uint16_t leaf_record_size;
    uint16_t nonleaf_record_size;
};

// TODO: 修改一个 Key 后可能要修改外键 & Index
// TODO: VARCHAR

class Database; // FIXME: Del   

class Sheet {
public:
    char name[MAX_NAME_LEN];
    char comment[MAX_COMMENT_LEN];
    Database* db;
    FileManager* fm;
    BufPageManager* bpm;
    int col_num = 0;
    int row_num = 0;
    Type col_ty[MAX_COL_NUM];
    ForeignKey f_key[MAX_COL_NUM];
    int index_num = 0;
    Index index_pt[MAX_INDEX_NUM];
    int record_num = 0;
    Sheet(Database* db, const char* name, int col_num, Type* col_ty) {
        this->db = db;
        this->fm = db->fm;
        this->bpm = db->bpm;
        memcpy(this->name, name, sizeof(name));
        this->col_num = col_num;
        memcpy(this->col_ty, col_ty, sizeof(Type) * col_num);
    }
};

class Database {
public:
    char name[MAX_NAME_LEN];
    Sheet* sheet[MAX_SHEET_NUM];
    FileManager* fm;
    BufPageManager* bpm;

    Database(const char* name) {
        memcpy(this->name, name, sizeof(name)); 
        memset(sheet, 0, sizeof(sheet));
        fm = new FileManager();
        bpm = new BufPageManager(fm);
        // TODO: If not, create, else open
    }

    ~Database() {
        delete fm;
        delete bpm;
        for (int i = 0; i < MAX_SHEET_NUM; i++) if (sheet[i] != nullptr) {
            delete sheet[i];
        }
    }

    Sheet* createSheet(const char* name) {
        // TODO: if too much
        return NULL;
    } 

private:
    void open(); 
    // TODO: Open a database

    void create();
    // TODO: New Folder
    // TODO: cd New Folder

};

int main() {
    // Init
    Database *db = new Database("TestDatabase"); // New Database
    Sheet *sheet = db->createSheet("TestSheet");
    sheet->insertRecord(0, nullptr);
    sheet->removeRecord(0, nullptr);
    sheet->updateRecord(0, nullptr);
    sheet->queryRecord(0, nullptr);
    
    // Open a database
    // Create a sheet
    // Add a record
}