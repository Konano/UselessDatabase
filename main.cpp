// TODO: makefile
// TODO: all define should be in a file

#include "constants.h"
#include "Any.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include "Database.h"
#include "Sheet.h"
#include "Type.h"
#include <iostream>
using namespace std;


// struct IndexRecord {
//     uint16_t NEXT_DEL_RECORD;
//     uint RECORD_ID;
//     uint8_t KEY; // TODO: its length is not constant
// };

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



// struct Record {
//     int recordID;
//     Any* data;
// };

// struct ForeignKey {
//     // TODO:
// };

// TODO: 修改一个 Key 后可能要修改外键 & Index
// TODO: VARCHAR

int main() {

    // Database *db = new Database("TestDatabase", true); // New Database
    // Sheet *sheet = db->createSheet("TestSheet", 4, new Type[4]{Type("Number"), Type("Name", enumType::CHAR, 3), Type("Height"), Type("Weigh")});
    // sheet->insertRecord(4, new Any[4]{2017011474, (char*)"ZLK", 160, 80});
    // sheet->removeRecord(0);

    Database *db = new Database("TestDatabase", false); // New Database
    Sheet *sheet = db->openSheet("TestSheet");
    sheet->insertRecord(4, new Any[4]{2017011474, (char*)"ZLK", 160, 80});
    sheet->createIndex(0);
    sheet->updateRecord(1,4,new Any[4]{2017011474, (char*)"ZLK", 160, 60});
    Any* ans;
    sheet->queryRecord(1,4,ans);

    //cout << "check" << endl;
    for(int i = 0;i < 4;i ++){
        if (ans[i].anyCast<int>() != NULL) {
            cout << ans[i].anyRefCast<int>() << endl;
        }
        if (ans[i].anyCast<char*>() != NULL) {
            cout << *ans[i].anyCast<char*>() << endl;
        }
    }

    delete db;
}