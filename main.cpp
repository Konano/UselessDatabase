#include "constants.h"
#include "parser.h"
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

extern int cleanFiles(const char *dir);
extern int cleanDir(const char *dir);

int yyparse (void);

void test_0() { // Testcase: new database
    assert(cleanFiles("TestDatabase") == 0);

    Database *db = new Database("TestDatabase", true);
    Sheet *sheet = db->createSheet("TestSheet", 4, new Type[4]{Type("Number"), Type("Name", enumType::CHAR, 3), Type("Height"), Type("Weigh")});
    sheet->insertRecord(new Any[4]{2017011474, (char*)"ZLK", 160, 80});
    assert(sheet->removeRecord(0) == 0);
    assert(sheet->removeRecord(0) == -1);

    delete db;
    cout << "Pass Test 0" << endl;
}

void test_1() { // Testcase: record manager
    test_0();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");
    sheet->insertRecord(new Any[4]{2017011475, (char*)"GTT", 170, 60});
    sheet->insertRecord(new Any[4]{2017011473, (char*)"LTF", 180, 70});
    sheet->insertRecord(new Any[4]{2017011470, (char*)"ZLK", 190, 50});
    assert(sheet->removeRecord(3) == 0);
    sheet->updateRecord(2, 4, new Any[4]{2017011474, (char*)"ZLK", 160, 60});
    Any* ans;
    assert(sheet->queryRecord(0, ans) == -1);
    assert(sheet->queryRecord(3, ans) == -1);
    assert(sheet->queryRecord(2, ans) == 0);
    assert(ans[0] == 2017011474);
    assert(ans[1] == "ZLK");
    assert(ans[2] == 160);
    assert(ans[3] == 60);
    assert(sheet->queryRecord(1, ans) == 0);
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

    Index ax(sheet, "haha", 1,3);

    ax.convert_BtreeNode_to_buf(&a);
    ax.convert_buf_to_BtreeNode(a.index);

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


void test_4() {
    test_0();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");

    sheet->createIndex(0);
    cout << "check" << endl;
    sheet->insertRecord(new Any[4]{2017011475, (char*)"GGT", 345, 34});
    sheet->insertRecord(new Any[4]{634645345, (char*)"KLE", 345, 34});
    cout << "check" << endl;
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}) == 1);
    assert(sheet->index[0].queryRecord(new Any[1]{634645345}) == 2);
    cout << "check" << endl;
    assert(sheet->removeRecord(1) == 0);
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}) == -1);
    assert(sheet->index[0].queryRecord(new Any[1]{634645345}) == 2);
    sheet->insertRecord(new Any[4]{4523524, (char*)"YNT", 345, 34});
    sheet->insertRecord(new Any[4]{87674234, (char*)"VWH", 345, 34});
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}) == -1);
    assert(sheet->index[0].queryRecord(new Any[1]{634645345}) == 2);
    assert(sheet->index[0].queryRecord(new Any[1]{4523524}) == 3);
    assert(sheet->index[0].queryRecord(new Any[1]{87674234}) == 4);

    delete db;
    cout << "Pass Test 4" << endl;
}

void test_5() {
    test_1();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");

    sheet->createIndex(0);
    //sheet->index[0].Debug();
    sheet->index[0].insertRecord(new Any[1]{2017011476}, 7);
    //sheet->index[0].Debug();
    sheet->index[0].insertRecord(new Any[1]{346453455}, 3);
    //sheet->index[0].Debug();
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}) == 1);
    assert(sheet->index[0].queryRecord(new Any[1]{346453455}) == 3);
    assert(sheet->index[0].queryRecord(new Any[1]{4523524}) == -1);
    assert(sheet->index[0].queryRecord(new Any[1]{87674234}) == -1);
    //sheet->index[0].Debug();
    sheet->index[0].removeRecord(new Any[1]{2017011475}, 1);
    //sheet->index[0].Debug();
    //assert(sheet->removeRecord(3) == 0);
    //assert(sheet->removeRecord(4) == 0);
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}) == -1);
    assert(sheet->index[0].queryRecord(new Any[1]{2017011476}) == 7);
    //assert(sheet->index[0].queryRecord(1, new Any[1]{4523524}) == -1);
    //assert(sheet->index[0].queryRecord(1, new Any[1]{87674234}) == -1);
    // sheet->removeIndex(0);

    delete db;
    cout << "Pass Test 5" << endl;
}

void test_6() {
    //test_0();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");

    sheet->createIndex(1);
    //cout << "check" << endl;
    sheet->insertRecord(new Any[4]{2017011475, (char*)"GGT", 345, 34});
    sheet->insertRecord(new Any[4]{634645345, (char*)"KLE", 345, 34});
    //cout << "check" << endl;
    assert(sheet->index[0].queryRecord(new Any[1]{(char*)"GGT"}) == 1);
    assert(sheet->index[0].queryRecord(new Any[1]{(char*)"KLE"}) == 2);
    //cout << "check" << endl;
    assert(sheet->removeRecord(1) == 0);
    assert(sheet->index[0].queryRecord(new Any[1]{(char*)"GGT"}) == -1);
    assert(sheet->index[0].queryRecord(new Any[1]{(char*)"KLE"}) == 2);
    sheet->insertRecord(new Any[4]{4523524, (char*)"YNT", 345, 34});
    sheet->insertRecord(new Any[4]{87674234, (char*)"VWH", 345, 34});
    assert(sheet->index[0].queryRecord(new Any[1]{(char*)"GGT"}) == -1);
    assert(sheet->index[0].queryRecord(new Any[1]{(char*)"KLE"}) == 2);
    assert(sheet->index[0].queryRecord(new Any[1]{(char*)"YNT"}) == 3);
    assert(sheet->index[0].queryRecord(new Any[1]{(char*)"VWH"}) == 4);

    delete db;
    cout << "Pass Test 6" << endl;
}

void init() { // Testcase: new database
    assert(cleanFiles("TestDatabase") == 0);

    Database *db = new Database("TestDatabase", true);
    db->createSheet("TestSheet", 4, new Type[4]{Type("Number"), Type("Name", enumType::CHAR, 3), Type("Height"), Type("Weigh")});

    delete db;
    cout << "Database init" << endl;
}

void test_7() { // add column, del column
    init();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");

    sheet->insertRecord(new Any[4]{2017011475, (char*)"GGT", 345, 34});
    sheet->insertRecord(new Any[4]{634645345, (char*)"KLE", 345, 34});
    sheet->createColumn(Type("Money", enumType::INT, 0, enumKeyType::Common, 0, Any(5)));
    sheet->removeColumn(0);
    sheet->modifyColumn(1, Type("Height", enumType::INT, 0, enumKeyType::Common, 0, Any(9)));

    delete db;
    cout << "Pass Test 7" << endl;
}

void init_2sheets() { // Testcase: new database
    assert(cleanFiles("TestDatabase") == 0);

    Database *db = new Database("TestDatabase", true);
    assert(db->createSheet("StudentInfo", 4, new Type[4]{Type("ID", enumType::INT, 0, enumKeyType::Primary), Type("Name", enumType::CHAR, 3), Type("Height"), Type("Weigh")}));
    assert(db->createSheet("ClassInfo", 2, new Type[2]{Type("ID", enumType::INT, 0, enumKeyType::Primary), Type("Name", enumType::CHAR, 3)}));
    assert(db->createSheet("Combine", 2, new Type[2]{Type("ClassID", enumType::INT, 0, enumKeyType::Foreign, 1), Type("StudentID", enumType::INT, 0, enumKeyType::Foreign, 0)}));

    delete db;
    cout << "Database init" << endl;
}

void test_8() {
    init_2sheets();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet_0 = db->openSheet("StudentInfo");
    Sheet *sheet_1 = db->openSheet("ClassInfo");
    Sheet *sheet_2 = db->openSheet("Combine");

    delete db;

    db = new Database("TestDatabase", false);
    delete db;

    cout << "Pass Test 8" << endl;
}

int main() {
    // test_0();
    // test_1();
    // test_2();
    // test_3();
    // test_4();
    // test_5();
    // test_6();
    test_7();
    // test_8();
    yyparse();
    return 0;
}
