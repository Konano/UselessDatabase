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
#include <fstream>
#include <sstream>
#include <assert.h>
using namespace std;

extern int cleanDatabase(const char *dbname);

int yyparse (void);

void test_0() { // Testcase: new database
    assert(cleanDatabase("TestDatabase") == 0);

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
/*
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

    vector <uint> key;
    key.push_back(0);

    sheet->createIndex(key,"test_index");
    sheet->insertRecord(new Any[4]{2017011475, (char*)"GGT", 345, 34});
    sheet->insertRecord(new Any[4]{634645345, (char*)"KLE", 345, 34});
    sheet->insertRecord(new Any[4]{634645345, (char*)"KLEX", 3456, 345});
    std::vector<int> v = sheet->index[0].queryRecord(new Any[1]{2017011475});
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}).size() == 1);
    assert(sheet->index[0].queryRecord(new Any[1]{634645345}).size() == 2);
    assert(sheet->removeRecord(1) == 0);
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}).size() == 0);
    assert(sheet->index[0].queryRecord(new Any[1]{634645345}).size() == 2);
    sheet->insertRecord(new Any[4]{4523524, (char*)"YNT", 345, 34});
    sheet->insertRecord(new Any[4]{87674234, (char*)"VWH", 345, 34});
    assert(sheet->index[0].queryRecord(new Any[1]{2017011475}).size() == 0);
    assert(sheet->index[0].queryRecord(new Any[1]{634645345}).size() == 2);
    assert(sheet->index[0].queryRecord(new Any[1]{4523524}).size() == 1);
    assert(sheet->index[0].queryRecord(new Any[1]{87674234}).size() == 1);

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
    //assert(sheet->index[0].queryRecord(new Any[1]{2017011475}) == 1);
    //assert(sheet->index[0].queryRecord(new Any[1]{346453455}) == 3);
    //assert(sheet->index[0].queryRecord(new Any[1]{4523524}) == -1);
    //assert(sheet->index[0].queryRecord(new Any[1]{87674234}) == -1);
    //sheet->index[0].Debug();
    sheet->index[0].removeRecord(new Any[1]{2017011475}, 1);
    //sheet->index[0].Debug();
    //assert(sheet->removeRecord(3) == 0);
    //assert(sheet->removeRecord(4) == 0);
    //assert(sheet->index[0].queryRecord(new Any[1]{2017011475}) == -1);
    //assert(sheet->index[0].queryRecord(new Any[1]{2017011476}) == 7);
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
    //assert(sheet->index[0].queryRecord(new Any[1]{(char*)"GGT"}) == 1);
    //assert(sheet->index[0].queryRecord(new Any[1]{(char*)"KLE"}) == 2);
    //cout << "check" << endl;
    //assert(sheet->removeRecord(1) == 0);
    //assert(sheet->index[0].queryRecord(new Any[1]{(char*)"GGT"}) == -1);
    //assert(sheet->index[0].queryRecord(new Any[1]{(char*)"KLE"}) == 2);
    sheet->insertRecord(new Any[4]{4523524, (char*)"YNT", 345, 34});
    sheet->insertRecord(new Any[4]{87674234, (char*)"VWH", 345, 34});
    //assert(sheet->index[0].queryRecord(new Any[1]{(char*)"GGT"}) == -1);
    //assert(sheet->index[0].queryRecord(new Any[1]{(char*)"KLE"}) == 2);
    //assert(sheet->index[0].queryRecord(new Any[1]{(char*)"YNT"}) == 3);
    //assert(sheet->index[0].queryRecord(new Any[1]{(char*)"VWH"}) == 4);

    delete db;
    cout << "Pass Test 6" << endl;
}
*/

void init() { // Testcase: new database
    assert(cleanDatabase("TestDatabase") == 0);

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
    // sheet->createColumn(Type("Money", enumType::INT, 0, enumKeyType::Common, 0, Any(5)));
    sheet->removeColumn(0);
    // sheet->modifyColumn(1, Type("Height", enumType::INT, 0, enumKeyType::Common, 0, Any(9)));

    delete db;
    cout << "Pass Test 7" << endl;
}

void init_2sheets() { // Testcase: new database
    assert(cleanDatabase("TestDatabase") == 0);

    Database *db = new Database("TestDatabase", true);
    // assert(db->createSheet("StudentInfo", 4, new Type[4]{Type("ID", enumType::INT, 0, enumKeyType::Primary), Type("Name", enumType::CHAR, 3), Type("Height"), Type("Weigh")}));
    // assert(db->createSheet("ClassInfo", 2, new Type[2]{Type("ID", enumType::INT, 0, enumKeyType::Primary), Type("Name", enumType::CHAR, 3)}));
    // assert(db->createSheet("Combine", 2, new Type[2]{Type("ClassID", enumType::INT, 0, enumKeyType::Foreign, 1), Type("StudentID", enumType::INT, 0, enumKeyType::Foreign, 0)}));

    delete db;
    cout << "Database init" << endl;
}

void test_8() {
    init_2sheets();

    Database *db = new Database("TestDatabase", false);
    db->openSheet("StudentInfo");
    db->openSheet("ClassInfo");
    db->openSheet("Combine");

    delete db;

    db = new Database("TestDatabase", false);
    delete db;

    cout << "Pass Test 8" << endl;
}

void test_9() { // varchar data decimal
    assert(cleanDatabase("TestDatabase") == 0);

    Database *db = new Database("TestDatabase", true);
    db->createSheet("TestSheet", 4, new Type[4]{Type("Number"), Type("Name", VARCHAR, 20), Type("Height", DECIMAL), Type("Birthday", DATE)});

    delete db;
    cout << "Database init" << endl;

    db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");

    sheet->insertRecord(new Any[4]{2017011475, (char*)"NanoApe", (long double)160.34, (uint32_t)20170101});
    sheet->insertRecord(new Any[4]{634645345, (char*)"Konano", (long double)177, (uint32_t)19990501});

    delete db;
    cout << "Pass Test 9" << endl;
}

inline uint32_t date_to_uint(string str) {
    str = str.substr(0,4) + str.substr(5,2) + str.substr(8,2);
    return (uint32_t)atoi(str.c_str());
}

char* copyStr(const char* _str) {
    uint size = strlen(_str);
    char* str = new char[size + 1];
    strcpy(str, _str);
    str[size] = '\0';
    return str;
}

void import_data(Sheet* sheet, const char* filename, char separator) {
    ifstream inFile(filename, ios::in);
    string line;
    string field;
    int cnt = 0;
    Any* data = new Any[sheet->col_num];
    // while (getline(inFile, line)) {
    //     istringstream sin(line); 
    // }
    // return;
    while (getline(inFile, line)) {
        istringstream sin(line); 
        memset(data, 0, sheet->col_num * sizeof(Any));
        for (uint i = 0; i < sheet->col_num; i++) {
            getline(sin, field, separator);  
            switch (sheet->col_ty[i].ty) {
            case INT:
                data[i] = atoi(field.c_str());
                break;
            case CHAR:
            case VARCHAR:
                data[i] = copyStr(field.c_str());
                break;
            case DATE:
                data[i] = date_to_uint(field);
                break;
            case DECIMAL:
                data[i] = strtold(field.c_str(), NULL);
                break;
            }
        }
        sheet->insertRecord(data);
        cnt++;
    }
    delete []data;
    inFile.close();
}

void import_data_tbl() {
    assert(cleanDatabase("orderdb") == 0);

    Database *db = new Database("orderdb", true);
    Sheet* sheet;

    // id = 0
    sheet = db->createSheet("part", 9, new Type[9]{
        Type("P_PARTKEY", INT), 
        Type("P_NAME", VARCHAR, 55), 
        Type("P_MFGR", CHAR, 25),
        Type("P_BRAND", CHAR, 10), 
        Type("P_TYPE", VARCHAR, 25), 
        Type("P_SIZE", INT), 
        Type("P_CONTAINER", CHAR, 10), 
        Type("P_RETAILPRICE", DECIMAL), 
        Type("P_COMMENT", VARCHAR, 23),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    import_data(sheet, "data/part.tbl",'|');

    // id = 1
    sheet = db->createSheet("region", 3, new Type[3]{
        Type("R_REGIONKEY", INT), 
        Type("R_NAME", CHAR, 25), 
        Type("R_COMMENT", VARCHAR, 152),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    import_data(sheet, "data/region.tbl",'|');

    // id = 2
    sheet = db->createSheet("nation", 4, new Type[4]{
        Type("N_NATIONKEY", INT), 
        Type("N_NAME", CHAR, 25), 
        Type("N_REGIONKEY", INT, 0, NULL, false),
        Type("N_COMMENT", VARCHAR, 152),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{2}), db->sheet[1]->p_key) == 0);
    import_data(sheet, "data/nation.tbl",'|');

    // id = 3
    sheet = db->createSheet("supplier", 7, new Type[7]{
        Type("S_SUPPKEY", INT), 
        Type("S_NAME", CHAR, 25), 
        Type("S_ADDRESS", VARCHAR, 40),
        Type("S_NATIONKEY", INT, 0, NULL, false),
        Type("S_PHONE", CHAR, 15),
        Type("S_ACCTBAL", DECIMAL),
        Type("S_COMMENT", VARCHAR, 101),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{3}), db->sheet[2]->p_key) == 0);
    import_data(sheet, "data/supplier.tbl",'|');
    import_data(sheet, "data/region.tbl",'|');

    // id = 4
    sheet = db->createSheet("customer", 8, new Type[8]{
        Type("C_CUSTKEY", INT), 
        Type("C_NAME", CHAR, 25), 
        Type("C_ADDRESS", VARCHAR, 40),
        Type("C_NATIONKEY", INT, 0, NULL, false),
        Type("C_PHONE", CHAR, 15),
        Type("C_ACCTBAL", DECIMAL),
        Type("C_MKTSEGMENT", CHAR, 10),
        Type("C_COMMENT", VARCHAR, 117),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{3}), db->sheet[2]->p_key) == 0);
    import_data(sheet, "data/customer.tbl",'|');

    // id = 5
    sheet = db->createSheet("partsupp", 5, new Type[5]{
        Type("PS_PARTKEY", INT, 0, NULL, false), 
        Type("PS_SUPPKEY", INT, 0, NULL, false), 
        Type("PS_AVAILQTY", INT),
        Type("PS_SUPPLYCOST", DECIMAL), 
        Type("PS_COMMENT", VARCHAR, 199),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 2, new int[2]{0, 1})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{0}), db->sheet[0]->p_key) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{1}), db->sheet[3]->p_key) == 0);
    import_data(sheet, "data/partsupp.tbl",'|');
    import_data(sheet, "data/customer.tbl",'|');

    // id = 6
    sheet = db->createSheet("orders", 9, new Type[9]{
        Type("O_ORDERKEY", INT), 
        Type("O_CUSTKEY", INT, 0, NULL, false), 
        Type("O_ORDERSTATUS", CHAR, 1),
        Type("O_TOTALPRICE", DECIMAL), 
        Type("O_ORDERDATE", DATE),
        Type("O_ORDERPRIORITY", CHAR, 15),
        Type("O_CLERK", CHAR, 15),
        Type("O_SHIPPRIORITY", INT),
        Type("O_COMMENT", VARCHAR, 79),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{1}), db->sheet[4]->p_key) == 0);
    import_data(sheet, "data/orders.tbl",'|');

    // id = 7
    sheet = db->createSheet("lineitem", 16, new Type[16]{
        Type("L_ORDERKEY", INT, 0, NULL, false), 
        Type("L_PARTKEY", INT, 0, NULL, false), 
        Type("L_SUPPKEY", INT, 0, NULL, false),
        Type("L_LINENUMBER", INT), 
        Type("L_QUANTITY", DECIMAL),
        Type("L_EXTENDEDPRICE", DECIMAL),
        Type("L_DISCOUNT", DECIMAL),
        Type("L_TAX", DECIMAL),
        Type("L_RETURNFLAG", CHAR, 1),
        Type("L_LINESTATUS", CHAR, 1),
        Type("L_SHIPDATE", DATE),
        Type("L_COMMITDATE", DATE),
        Type("L_RECEIPTDATE", DATE),
        Type("L_SHIPINSTRUCT", CHAR, 25),
        Type("L_SHIPMODE", CHAR, 10),
        Type("L_COMMENT", VARCHAR, 44),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 2, new int[2]{0, 3})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{0}), db->sheet[6]->p_key) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 2, new int[2]{1, 2}), db->sheet[5]->p_key) == 0);
    import_data(sheet, "data/lineitem.tbl",'|');

    delete db;
}

void import_data_csv() {
    assert(cleanDatabase("orderdb") == 0);

    Database *db = new Database("orderdb", true);
    Sheet* sheet;

    // id = 0
    sheet = db->createSheet("part", 9, new Type[9]{
        Type("P_PARTKEY", INT), 
        Type("P_NAME", VARCHAR, 55), 
        Type("P_MFGR", CHAR, 25),
        Type("P_BRAND", CHAR, 10), 
        Type("P_TYPE", VARCHAR, 25), 
        Type("P_SIZE", INT), 
        Type("P_CONTAINER", CHAR, 10), 
        Type("P_RETAILPRICE", DECIMAL), 
        Type("P_COMMENT", VARCHAR, 23),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    import_data(sheet, "data/part.csv",',');

    // id = 1
    sheet = db->createSheet("region", 3, new Type[3]{
        Type("R_REGIONKEY", INT), 
        Type("R_NAME", CHAR, 25), 
        Type("R_COMMENT", VARCHAR, 152),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    import_data(sheet, "data/region.csv",',');

    // id = 2
    sheet = db->createSheet("nation", 4, new Type[4]{
        Type("N_NATIONKEY", INT), 
        Type("N_NAME", CHAR, 25), 
        Type("N_REGIONKEY", INT, 0, NULL, false),
        Type("N_COMMENT", VARCHAR, 152),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{2}), db->sheet[1]->p_key) == 0);
    import_data(sheet, "data/nation.csv",',');

    // id = 3
    sheet = db->createSheet("supplier", 7, new Type[7]{
        Type("S_SUPPKEY", INT), 
        Type("S_NAME", CHAR, 25), 
        Type("S_ADDRESS", VARCHAR, 40),
        Type("S_NATIONKEY", INT, 0, NULL, false),
        Type("S_PHONE", CHAR, 15),
        Type("S_ACCTBAL", DECIMAL),
        Type("S_COMMENT", VARCHAR, 101),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{3}), db->sheet[2]->p_key) == 0);
    import_data(sheet, "data/supplier.csv",',');
    import_data(sheet, "data/region.csv",',');

    // id = 4
    sheet = db->createSheet("customer", 8, new Type[8]{
        Type("C_CUSTKEY", INT), 
        Type("C_NAME", CHAR, 25), 
        Type("C_ADDRESS", VARCHAR, 40),
        Type("C_NATIONKEY", INT, 0, NULL, false),
        Type("C_PHONE", CHAR, 15),
        Type("C_ACCTBAL", DECIMAL),
        Type("C_MKTSEGMENT", CHAR, 10),
        Type("C_COMMENT", VARCHAR, 117),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{3}), db->sheet[2]->p_key) == 0);
    import_data(sheet, "data/customer.csv",',');

    // id = 5
    sheet = db->createSheet("partsupp", 5, new Type[5]{
        Type("PS_PARTKEY", INT, 0, NULL, false), 
        Type("PS_SUPPKEY", INT, 0, NULL, false), 
        Type("PS_AVAILQTY", INT),
        Type("PS_SUPPLYCOST", DECIMAL), 
        Type("PS_COMMENT", VARCHAR, 199),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 2, new int[2]{0, 1})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{0}), db->sheet[0]->p_key) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{1}), db->sheet[3]->p_key) == 0);
    import_data(sheet, "data/partsupp.csv",',');
    import_data(sheet, "data/customer.csv",',');

    // id = 6
    sheet = db->createSheet("orders", 9, new Type[9]{
        Type("O_ORDERKEY", INT), 
        Type("O_CUSTKEY", INT, 0, NULL, false), 
        Type("O_ORDERSTATUS", CHAR, 1),
        Type("O_TOTALPRICE", DECIMAL), 
        Type("O_ORDERDATE", DATE),
        Type("O_ORDERPRIORITY", CHAR, 15),
        Type("O_CLERK", CHAR, 15),
        Type("O_SHIPPRIORITY", INT),
        Type("O_COMMENT", VARCHAR, 79),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 1, new int[1]{0})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{1}), db->sheet[4]->p_key) == 0);
    import_data(sheet, "data/orders.csv",',');

    // id = 7
    sheet = db->createSheet("lineitem", 16, new Type[16]{
        Type("L_ORDERKEY", INT, 0, NULL, false), 
        Type("L_PARTKEY", INT, 0, NULL, false), 
        Type("L_SUPPKEY", INT, 0, NULL, false),
        Type("L_LINENUMBER", INT), 
        Type("L_QUANTITY", DECIMAL),
        Type("L_EXTENDEDPRICE", DECIMAL),
        Type("L_DISCOUNT", DECIMAL),
        Type("L_TAX", DECIMAL),
        Type("L_RETURNFLAG", CHAR, 1),
        Type("L_LINESTATUS", CHAR, 1),
        Type("L_SHIPDATE", DATE),
        Type("L_COMMITDATE", DATE),
        Type("L_RECEIPTDATE", DATE),
        Type("L_SHIPINSTRUCT", CHAR, 25),
        Type("L_SHIPMODE", CHAR, 10),
        Type("L_COMMENT", VARCHAR, 44),
    });
    assert(sheet->createPrimaryKey(new PrimaryKey(sheet, 2, new int[2]{0, 3})) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 1, new int[1]{0}), db->sheet[6]->p_key) == 0);
    assert(sheet->createForeignKey(new ForeignKey(sheet, 2, new int[2]{1, 2}), db->sheet[5]->p_key) == 0);
    import_data(sheet, "data/lineitem.csv",',');

    delete db;
}

Database* db;

int main() {
    // test_0();
    // test_1();
    // test_2();
    // test_3();
    // test_4();
    // test_5();
    // test_6();
    // test_7();
    // test_8();
    // test_9();
    // import_data_csv();
    // import_data_tbl();
    yyparse();
    return 0;
}
