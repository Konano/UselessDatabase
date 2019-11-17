#include "Index.h"

#include "BufPageManager.h"
#include "Sheet.h"
#include "FileManager.h"
#include "Database.h"

#include <vector>
using namespace std;

extern char* Dir(const char* dir, const char* filename, const char* name, const char* suffix);

/*
struct Index {
public:
    uint32_t key; // Tree, Pointer, TODO: Finally will be index_cal
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

    Any calKeyValue(Record* record, uint32_t key);

public:

    Index() {
        // fm->openFile
    }

    // void insertRecord(const int len, Any* info) {
    //     if (main) {
    //         BTreeInsert(root_page, NULL, record);
    //     } else {
    //         BTreeInsert(root_page, calKeyValue(record, key), record);
    //     }
    // }
};
*/

// uint32_t key; // Tree, Pointer, TODO: Finally will be index_cal
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
    j["key"] = key;
    j["page_num"] = page_num;
    j["root_page"] = root_page;
    j["next_del_page"] = next_del_page;
    j["record_size"] = record_size;
    return j;
}

Index::Index(Sheet* sheet, const char* name, uint key) : sheet(sheet), key(key) {
    strcpy(this->name, name);
    sheet->fm->createFile(Dir(sheet->db->name, sheet->name, name, ".usid"));
    page_num = root_page = next_del_page = 0;
    record_size = sheet->col_ty[key].size() + 2 + 4;
}

Index::Index(Sheet* sheet, json j) : sheet(sheet) {
    strcpy(name, j["name"].get<std::string>().c_str());
    key = j["key"].get<int>();
    page_num = j["page_num"].get<int>();
    root_page = j["root_page"].get<int>();
    next_del_page = j["next_del_page"].get<int>();
    record_size = j["record_size"].get<int>();
}

void Index::open() {
    sheet->fm->openFile(Dir(sheet->db->name, sheet->name, name, ".usid"), fileID);
}

void Index::close() {
    sheet->fm->closeFile(fileID);
}

void Index::initIndex() {
    page_num = 1;
    root_page = 0;
    next_del_page = 1;
    //header = 10Byte
    //child ptr is one more
    rank = (PAGE_SIZE - 10 - 4) / (record_size + 4 + 2);

    int index;
    BufType buf = sheet->bpm->getPage(fileID, 0, index);
    *(uint32_t *)(buf+0) = 0;
    *(uint16_t *)(buf+4) = (uint16_t)-1;
    *(uint16_t *)(buf+6) = 18;
    *(uint16_t *)(buf+8) = 1;
}

BtreeNode Index::convert_buf_to_BtreeNode(int index){
    BtreeNode ans;
    ans.child.clear();
    ans.child_ptr.clear();
    ans.record.clear();
    ans.left_page = nullptr;
    ans.right_page = nullptr;
    
    ans.index = index;

    int _index;
    BufType buf = sheet->bpm->getPage(index, 0, _index);

    ans.left_page_index = *(uint32_t *)(buf + 4);
    ans.right_page_index = *(uint32_t *)(buf + 8);
    ans.record_cnt = *(uint16_t *)(buf + 12);
    ans.record_size = *(uint32_t *)(buf + 14);

    if(record_size != 0)ans.child.push_back(*(uint32_t *)(buf + 18));

    for (int i = 0;i < ans.record_cnt;i ++){
        Record temp;
        temp.record_id = *(uint32_t *)(buf + 22 + i * ans.record_size);
        if (this->ty == enumType::INT) {
            temp.key = *(int*)(buf + 22 + i * ans.record_size + 4);
        }
        if (this->ty == enumType::CHAR) {
            char* str = (char *)malloc((ans.record_size - 8 + 1) * sizeof(char));
            memcpy(str, buf + 22 + i * ans.record_size + 4 , ans.record_size - 8);
            str[ans.record_size - 8] = '\0';
            temp.key = str;
        }
        ans.child.push_back(*(uint32_t *)(buf + 22 + (i + 1) * ans.record_size - 4));
        ans.record.push_back(temp);
    }
    return ans;
}

void Index::convert_BtreeNode_to_buf(BtreeNode node){
    int _index;
    BufType buf = sheet->bpm->getPage(node.index, 0, _index);

    buf += 4; *(uint32_t*) buf = node.left_page_index;
    buf += 4; *(uint32_t*) buf = node.right_page_index;
    buf += 4; *(uint16_t*) buf = node.record_cnt;
    buf += 2; *(uint32_t*) buf = node.record_size;

    if(node.record_cnt != 0){
        buf += 4; *(uint32_t*) buf = node.child[0];
    }

    for (int i = 0;i < node.record_cnt;i ++){
        buf += 4; *(uint32_t*) buf = node.record[i].record_id;
        buf += 4;
        if (node.record[i].key.anyCast<int>() != NULL) {
            *(uint32_t*)buf = *node.record[i].key.anyCast<int>();
        }
        if (node.record[i].key.anyCast<char*>() != NULL) {
            memcpy(buf, *node.record[i].key.anyCast<char*>(), node.record_size - 8);
        }
        buf += node.record_size - 8; *(uint32_t*) buf = node.child[i + 1];
    }
}

/*

int searchRecord(const int _root, const int len, Any* info)
{
    int v = _root;    
    int _hot = - 1;

    int index;
    BufType buf = this.sheet->bpm->getPage(fileID, 0, index);

    while (v) {
        int l = 0,r = (*(uint16_t *)(buf + 8) ) >> 1;
        if(r == 0) return -1;
        
        int ans = -1;
        while(l <= r){
            int mid = (l + r) >> 1;
            if(this->sheet->col_ty[key].ty == enumType::INT){
                int tmp = *(int*)(buf+10+record_size*mid);
                int info_val = info->anyRefCast<int>();

                if(info_val >= tmp){
                    ans = mid;
                    l = mid + 1;
                }
                else r = mid - 1;
            }
            else if(this->sheet->col_ty[key].ty == enumType::CHAR){
                char* tmp = (char *)malloc((sheet->col_ty[key].len+1) * sizeof(char));
                tmp[sheet->col_ty[key].len] = '\0';
                char* info_val = info.anyCast<char*>();
                break;
                ans = -1;
            }
        }


        int r = ans;    //在当前节点中，找到不大于e的最大关键码
        if(this.sheet->col_ty[key].ty == enumType::INT){
            int info_val = info->anyRefCast<int>();
            if ((0 <= r) && (info_val == *(int*)(buf+10+record_size*r);))
                return v;   //成功：在当前节点中命中目标关键码
        }
        _hot = v;   
        v = *(uint32_t *)(buf+10 + record_size*rank + 4 * r);    //否则，转入对应子树（_hot指向其父）----需做I/O，最费时间
    }
 
    return -1;
}
*/

/*
void BTree<T>::solveOverflow(BTNodePosi(T) v)
{
    if (_order >= v->child.size())  return; //递归基：当前节点并未上溢
    int s = _order / 2; //轴点（此时应有_order = key.size() = child.size() - 1）
    BTNodePosi(T) u = new BTNode<T>();  //注意：新节点已有一个空孩子
    for (int j = 0; j < _order - s - 1; ++j) {  //v右侧_order-s-1个孩子及关键码分裂为右侧节点u
        VecInsert(u->child, j, VecRemove(v->child, s + 1)); //逐个移动效率低
        VecInsert(u->key, j, VecRemove(v->key, s + 1)); //此策略可改进
    }
    u->child[_order - s - 1] = VecRemove(v->child, s + 1);  //移动v最靠右的孩子
    if (u->child[0])    //若u的孩子们非空
        for (int j = 0; j < _order - s; ++j)    //令它们的父节点统一
            u->child[j]->parent = u;    //指向u
    BTNodePosi(T) p = v->parent;    //v当前的父节点p
    if (!p) {   //若p空则创建之
        _root = p = new BTNode<T>();
        p->child[0] = v;
        v->parent = p;
    }
    int r = 1 + VecSearch(p->key, v->key[0]);   //p中指向v的指针的秩
    VecInsert(p->key, r, VecRemove(v->key, s)); //轴点关键码上升
    VecInsert(p->child, r + 1, u);  u->parent = p;  //新节点u与父节点p互联
    solveOverflow(p);   //上升一层，如有必要则继续分裂----至多递归O（logn）层
}
*/

void Index::insertRecord(const int len, Any* info) {
    if (page_num == 0) {
        initIndex();
    }
    /*
    BTNodePosi(T) v = search(e);    if (v) return false;    //确认目标节点不存在
    int r = VecSearch(_hot->key, e);    //在节点_hot的有序关键码向量中查找合适的插入位置
    VecInsert(_hot->key, r + 1, e); //将新关键码插至对应的位置
    VecInsert(_hot->child, r + 2, (BTNodePosi(T))NULL); //创建一个空子树指针
    _size++;    //更新全树规模
    solveOverflow(_hot);    //如有必要，需做分裂
    return true;    //插入成功*/
}



void Index::removeRecord(const int len, Any* info) {

}