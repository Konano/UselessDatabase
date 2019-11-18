// TODO remove using namespace std;

#include "constants.h"
#include "Any.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include "Database.h"
#include "Sheet.h"
#include "Type.h"
#include "Index.h"
#include <iostream>
using namespace std;

// #define NDEBUG
#include <assert.h>

extern int cleanDir(const char *dir);

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

// struct ForeignKey {
//     // TODO:
// };

// TODO: 修改一个 Key 后可能要修改外键 & Index
// TODO: VARCHAR

void test_0() { // Testcase: new database
    assert(cleanDir("TestDatabase") == 0);

    Database *db = new Database("TestDatabase", true);
    Sheet *sheet = db->createSheet("TestSheet", 4, new Type[4]{Type("Number"), Type("Name", enumType::CHAR, 3), Type("Height"), Type("Weigh")});
    sheet->insertRecord(4, new Any[4]{2017011474, (char*)"ZLK", 160, 80});
    assert(sheet->removeRecord(0) == 0);
    assert(sheet->removeRecord(0) == -1);

    delete db;
    cout << "Pass Test 0" << endl;
}

void test_1() { // Testcase: record manager
    test_0();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");
    sheet->insertRecord(4, new Any[4]{2017011475, (char*)"GTT", 170, 60});
    sheet->insertRecord(4, new Any[4]{2017011473, (char*)"LTF", 180, 70});
    sheet->insertRecord(4, new Any[4]{2017011470, (char*)"ZLK", 190, 50});
    assert(sheet->removeRecord(3) == 0);
    sheet->updateRecord(2, 4, new Any[4]{2017011474, (char*)"ZLK", 160, 60});
    Any* ans;
    assert(sheet->queryRecord(0, 4, ans) == -1);
    assert(sheet->queryRecord(3, 4, ans) == -1);
    assert(sheet->queryRecord(2, 4, ans) == 0);
    assert(ans[0] == 2017011474);
    assert(ans[1] == "ZLK");
    assert(ans[2] == 160);
    assert(ans[3] == 60);
    assert(sheet->queryRecord(1, 4, ans) == 0);
    assert(ans[0] == 2017011475);
    assert(ans[1] == "GTT");
    assert(ans[2] == 170);
    assert(ans[3] == 60);

    delete db;
    cout << "Pass Test 1" << endl;
}

void test_2() {
    test_1();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");

    //sheet->createIndex(0);

    //cout << "check" << endl;

    BtreeNode a;
    a.left_page_index = -1;
    a.right_page_index = -1;
    a.index = 0;
    a.record_cnt = 2;
    a.record.clear();
    a.child.clear();
    a.child.push_back(1);
    a.child.push_back(2);
    a.child.push_back(3);
    BtreeRecord temp;
    temp.record_id = 1;
    temp.key = 1;
    a.record.push_back(temp);
    temp.record_id = 2;
    a.record.push_back(temp);

    Index ax(sheet, "haha", 1,3,0);

    ax.convert_BtreeNode_to_buf(&a);
    BtreeNode* b = ax.convert_buf_to_BtreeNode(a.index);

    /*
    printf("%d\n", b.index);
    for (auto i: b.child){
        printf("%d\n",i);
    }
    for (int i = 0;i < 2;i ++){
        if (b.record[i].key.anyCast<int>() != NULL){
            cout << *b.record[i].key.anyCast<int>() << endl;
        }
    }
    */
    delete db;
    cout << "Pass Test 2" << endl;
}

void test_3() {
    test_1();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");

    sheet->createIndex(0);
    // assert(sheet->index[0].queryRecord(1, new Any[1]{2017011475}) == 1);
    // assert(sheet->index[0].queryRecord(1, new Any[1]{2017011474}) == 1);
    // sheet->removeIndex(0);

    delete db;
    cout << "Pass Test 3" << endl;
}

/*
void test_4() {
    test_0();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");

    sheet->createIndex(0);
    cout << "check" << endl;
    sheet->insertRecord(1, new Any[1]{2017011475});
    sheet->insertRecord(1, new Any[1]{6346453455});
    cout << "check" << endl;
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}) == 1);
    assert(sheet->index[0].queryRecord(new Any[1]{6346453455}) == 2);
    cout << "check" << endl;
    assert(sheet->removeRecord(1) == 0);
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}) == -1);
    assert(sheet->index[0].queryRecord(new Any[1]{6346453455}) == 2);
    sheet->insertRecord(4, new Any[4]{4523524, (char*)"GTK", 345, 34});
    sheet->insertRecord(4, new Any[4]{87674234, (char*)"GTK", 345, 34});
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}) == -1);
    assert(sheet->index[0].queryRecord(new Any[1]{6346453455}) == 2);
    assert(sheet->index[0].queryRecord(new Any[1]{4523524}) == 3);
    assert(sheet->index[0].queryRecord(new Any[1]{87674234}) == 4);

    delete db;
    cout << "Pass Test 4" << endl;
}

void test_5() {
    test_4();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");

    assert(sheet->index[0].queryRecord(1, new Any[1]{2017011475}) == -1);
    assert(sheet->index[0].queryRecord(1, new Any[1]{6346453455}) == 2);
    assert(sheet->index[0].queryRecord(1, new Any[1]{4523524}) == 3);
    assert(sheet->index[0].queryRecord(1, new Any[1]{87674234}) == 4);
    assert(sheet->removeRecord(2) == 0);
    assert(sheet->removeRecord(3) == 0);
    assert(sheet->removeRecord(4) == 0);
    assert(sheet->index[0].queryRecord(1, new Any[1]{2017011475}) == -1);
    assert(sheet->index[0].queryRecord(1, new Any[1]{6346453455}) == -1);
    assert(sheet->index[0].queryRecord(1, new Any[1]{4523524}) == -1);
    assert(sheet->index[0].queryRecord(1, new Any[1]{87674234}) == -1);
    sheet->removeIndex(0);

    delete db;
    cout << "Pass Test 5" << endl;
}
*/

int main() {
    // test_0();
    // test_1();
    // test_2();
    test_3();
    // test_4();
    // test_5();
}