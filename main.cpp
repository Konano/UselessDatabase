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

void test_0() {
    rmdir("TestDatabase");

    Database *db = new Database("TestDatabase", true);
    Sheet *sheet = db->createSheet("TestSheet", 4, new Type[4]{Type("Number"), Type("Name", enumType::CHAR, 3), Type("Height"), Type("Weigh")});
    sheet->insertRecord(4, new Any[4]{2017011474, (char*)"ZLK", 160, 80});
    sheet->removeRecord(0);

    delete db;
    cout << "Pass Test 0" << endl;
}

void test_1() {
    test_0();
    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");
    sheet->insertRecord(4, new Any[4]{2017011475, (char*)"GTT", 170, 60});
    sheet->insertRecord(4, new Any[4]{2017011473, (char*)"LTF", 180, 70});
    sheet->insertRecord(4, new Any[4]{2017011470, (char*)"ZLK", 190, 50});
    sheet->removeRecord(3);
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
    test_0();

    Database *db = new Database("TestDatabase", false);
    Sheet *sheet = db->openSheet("TestSheet");

    //sheet->createIndex(0);

    //cout << "check" << endl;

    BtreeNode a;
    a.left_page_index = -1;
    a.right_page_index = -1;
    a.index = 0;
    a.record_cnt = 2;
    a.record_size = 12;
    a.record.clear();
    a.child.clear();
    a.child.push_back(1);
    a.child.push_back(2);
    a.child.push_back(3);
    Record temp;
    temp.record_id = 1;
    temp.key = 1;
    a.record.push_back(temp);
    temp.record_id = 2;
    a.record.push_back(temp);

    Index ax(sheet,"haha",1);

    //cout << "check" << endl;
    ax.convert_BtreeNode_to_buf(a);
    //cout << "check" << endl;
    BtreeNode b = ax.convert_buf_to_BtreeNode(a.index);
    //cout << "check" << endl;

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


/*
void test_3(){
    Any a = 5;
    if (a < 4) printf("Yes\n"); else printf("No\n");
    if (a < 5) printf("Yes\n"); else printf("No\n");
    if (a < 6) printf("Yes\n"); else printf("No\n");
    if (a > 4) printf("Yes\n"); else printf("No\n");
    if (a > 5) printf("Yes\n"); else printf("No\n");
    if (a > 6) printf("Yes\n"); else printf("No\n");
    if (a == 4) printf("Yes\n"); else printf("No\n");
    if (a == 5) printf("Yes\n"); else printf("No\n");
    if (a == 6) printf("Yes\n"); else printf("No\n");
    if (a <= 4) printf("Yes\n"); else printf("No\n");
    if (a <= 5) printf("Yes\n"); else printf("No\n");
    if (a <= 6) printf("Yes\n"); else printf("No\n");
    if (a >= 4) printf("Yes\n"); else printf("No\n");
    if (a >= 5) printf("Yes\n"); else printf("No\n");
    if (a >= 6) printf("Yes\n"); else printf("No\n");
    Any b = (char*)"TEST";
    if (b < "TGR") printf("Yes\n"); else printf("No\n");
    if (b < "TDSFH") printf("Yes\n"); else printf("No\n");
    if (b < "TEST") printf("Yes\n"); else printf("No\n");
    if (b > "TGR") printf("Yes\n"); else printf("No\n");
    if (b > "TDSFH") printf("Yes\n"); else printf("No\n");
    if (b > "TEST") printf("Yes\n"); else printf("No\n");
    if (b == "TGR") printf("Yes\n"); else printf("No\n");
    if (b == "TDSFH") printf("Yes\n"); else printf("No\n");
    if (b == "TEST") printf("Yes\n"); else printf("No\n");
    if (b <= "TGR") printf("Yes\n"); else printf("No\n");
    if (b <= "TDSFH") printf("Yes\n"); else printf("No\n");
    if (b <= "TEST") printf("Yes\n"); else printf("No\n");
    if (b >= "TGR") printf("Yes\n"); else printf("No\n");
    if (b >= "TDSFH") printf("Yes\n"); else printf("No\n");
    if (b >= "TEST") printf("Yes\n"); else printf("No\n");
}
*/

int main() {
    // test_0();
    // test_1();
    test_2();
    // test_3();
}