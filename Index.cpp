#include "Index.h"

#include "BufPageManager.h"
#include "Sheet.h"
#include "FileManager.h"
#include "Database.h"
#include <iostream>

#include <vector>
using namespace std;

extern char* dirPath(const char* dir, const char* filename, const char* name, const char* suffix);

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

Index::Index(Sheet* sheet, const char* name, uint key,int btree_max_per_node,int btree_root_index) : sheet(sheet), key(key), btree_max_per_node(btree_max_per_node), btree_root_index(btree_root_index) {
    strcpy(this->name, name);
    sheet->fm->createFile(dirPath(sheet->db->name, sheet->name, name, ".usid"));
    page_num = 1;
    next_del_page = -1;
    record_size = sheet->col_ty[key].size() + 8;
    max_recond_num = (PAGE_SIZE - 18) / record_size;

    open();

    int index;
    BufType buf = sheet->bpm->getPage(fileID, 0, index);
    *(uint32_t *)(buf+0) = -1;
    *(uint32_t *)(buf+4) = -1;
    *(uint32_t *)(buf+8) = -1;
    *(uint16_t *)(buf+12) = 0;
    sheet->bpm->markDirty(index);
    
    root_page = 0;
    root = convert_buf_to_BtreeNode(root_page);
    close();
}

Index::Index(Sheet* sheet, json j) : sheet(sheet) {
    strcpy(name, j["name"].get<std::string>().c_str());
    key = j["key"].get<int>();
    page_num = j["page_num"].get<int>();
    root_page = j["root_page"].get<int>();
    next_del_page = j["next_del_page"].get<int>();
    record_size = j["record_size"].get<int>();
    fileID = -1; // file is closed
}

void Index::open() {
    sheet->fm->openFile(dirPath(sheet->db->name, sheet->name, name, ".usid"), fileID);
}

void Index::close() {
    sheet->fm->closeFile(fileID);
    fileID = -1; // file is closed
}

void Index::remove() {
    if (fileID != -1) close();
    Btree_remove(root);
}

void Index::Btree_remove(BtreeNode* node) {
    if (node == nullptr) return;
    Btree_remove(node->left_page);
    Btree_remove(node->right_page);
    if (node->record_cnt) {
        for (int i = 0; i <= node->record_cnt; i++) {
            Btree_remove(node->child_ptr[i]);
        }
    }
}

BtreeNode* Index::convert_buf_to_BtreeNode(int index) {
    BtreeNode* ans = new BtreeNode();
    ans->child.clear();
    ans->child_ptr.clear();
    ans->record.clear();
    ans->left_page = nullptr;
    ans->right_page = nullptr;
    
    ans->index = index;

    int _index;
    BufType buf = sheet->bpm->getPage(fileID, index, _index);

    ans->left_page_index = *(uint32_t *)(buf + 4);
    ans->right_page_index = *(uint32_t *)(buf + 8);
    ans->record_cnt = *(uint16_t *)(buf + 12);

    if(record_size != 0) ans->child.push_back(*(uint32_t *)(buf + 14));

    for (int i = 0;i < ans->record_cnt;i ++){
        BtreeRecord temp;
        temp.record_id = *(uint32_t *)(buf + 18 + i * record_size);
        if (this->ty == enumType::INT) {
            temp.key = *(int*)(buf + 18 + i * record_size + 4);
        }
        if (this->ty == enumType::CHAR) {
            char* str = (char *)malloc((record_size - 8 + 1) * sizeof(char));
            memcpy(str, buf + 18 + i * record_size + 4 , record_size - 8);
            str[record_size - 8] = '\0';
            temp.key = str;
        }
        ans->child.push_back(*(uint32_t *)(buf + 18 + (i + 1) * record_size - 4));
        ans->record.push_back(temp);
    }
    ans->fa_index = *(uint32_t *)(buf + 18 + ans->record_cnt * record_size);
    return ans;
}

void Index::convert_BtreeNode_to_buf(BtreeNode* node) {
    int _index;
    BufType buf = sheet->bpm->getPage(fileID, node->index, _index);

    *(uint32_t*) buf = -1; buf += 4; 
    *(uint32_t*) buf = node->left_page_index; buf += 4; 
    *(uint32_t*) buf = node->right_page_index; buf += 4; 
    *(uint16_t*) buf = node->record_cnt; buf += 2; 

    if(node->record_cnt != 0){
        *(uint32_t*) buf = node->child[0]; buf += 4;
    }

    for (int i = 0;i < node->record_cnt;i ++){
        *(uint32_t*) buf = node->record[i].record_id; buf += 4;
        if (node->record[i].key.anyCast<int>() != NULL) {
            *(uint32_t*)buf = *node->record[i].key.anyCast<int>();
        }
        if (node->record[i].key.anyCast<char*>() != NULL) {
            memcpy(buf, *node->record[i].key.anyCast<char*>(), record_size - 8);
        } buf += record_size - 8; 
        *(uint32_t*) buf = node->child[i + 1]; buf += 4;
    }
    sheet->bpm->markDirty(_index);
    buf += 4; *(uint32_t*) buf = node->fa_index;
}

int Index::queryRecord(Any* info) {
    return Index::queryRecord(info, root_page);
}

int Index::queryRecord(Any* info, int index) {
    
    //cout << "queryRecord";
    //cout << index << endl;
    printf("qu%d\n", index);
    BtreeNode *now = Index::convert_buf_to_BtreeNode(index);
    
    int i;
    for (i = 0;i < now->record_cnt;i ++){
        if(now->record[i].key == *info){
            return now->record[i].record_id;
        }
        else if(now->record[i].key > *info){
            break;
        }
    }

    if (now->is_leaf){
        return -1;
    }
    else {
        return Index::queryRecord(info, now->child[i]);
    }
}

void Index::overflow_upstream(int index){

    //cout << "overflowupstream";
    //cout << index << end;
    printf("up%d\n", index);
    BtreeNode* now = Index::convert_buf_to_BtreeNode(index);

    if ((int)now->record.size() <= btree_max_per_node - 1){
        return ;
    }
    else {

        BtreeNode* fa;

        if(now->fa_index == -1){
            fa = new BtreeNode();
            fa->index = ++page_num;
            fa->is_leaf = false;
        }
        else{
            fa = Index::convert_buf_to_BtreeNode(now->fa_index);
        }
        BtreeNode* right = new BtreeNode();
        //TODO: Reuse del page
        right->index = ++ page_num;


        BtreeRecord mid = now->record[now->record.size() >> 1];
        vector<BtreeRecord>::iterator it = fa->record.begin();
        vector<int>::iterator itx = fa->child.begin();
        uint i;
        for (i = 0;i < fa->record.size();i ++){
            if(fa->record[i].key >= mid.key){
                break;
            }
            it ++;
            itx ++;
        }
        fa->record.insert(it,mid);
        fa->child.insert(itx,-1);
        fa->child[i] = now->index;
        fa->child[i + 1] = right->index;
        now->fa_index = fa->index;
        right->fa_index = fa->index;
        right->child.clear();
        right->record.clear();
        right->child.push_back(now->child[(now->record.size() >> 1) + 1]);
        for (uint i = (now->record.size() >> 1) + 1;i < now->record.size();i ++){
            right->child.push_back(now->child[i + 1]);
            right->record.push_back(now->record[i]);
        }

        int left_size = (now->record.size() >> 1);
        int right_size = now->record.size() - left_size - 1;

        now->record_cnt = left_size;
        right->record_cnt = right_size;

        Index::convert_BtreeNode_to_buf(now);
        Index::convert_BtreeNode_to_buf(fa);
        Index::convert_BtreeNode_to_buf(right);

        overflow_upstream(fa->index);        
    }
}

void Index::insertRecord(Any* info, int record_id) {
    return insertRecord(info, record_id,root_page);
}

void Index::insertRecord(Any* info, int record_id,int index) {
    //cout << "insertRecord" ;
    //cout << index << end;
    printf("in%d\n", index);
    BtreeNode *now = Index::convert_buf_to_BtreeNode(index);
    
    int i = 0;
    vector<BtreeRecord>::iterator it;
    for(it=now->record.begin();it!=now->record.end();it++)
    {
        cout << i << endl;
        if(it->key >= *info){
            break;
        }
        i ++;
    }

    if (now->is_leaf){
        //cout << "shit2" << endl;
        if((int)now->record.size() > btree_max_per_node - 1){
            now->record.insert(it, BtreeRecord(record_id,*info));
            vector<int>::iterator itx = now->child.begin();
            while(i --)itx ++;
            now->child.insert(itx, -1);
            Index::convert_BtreeNode_to_buf(now);
            overflow_upstream(index);
            return ;
        }
        else {
            now->record.insert(it, BtreeRecord(record_id,*info));
            vector<int>::iterator itx = now->child.begin();
            while(i --)itx ++;
            now->child.insert(itx, -1);
            printf("%d\n", now->record.size());
            printf("%d\n", now->child.size());
            Index::convert_BtreeNode_to_buf(now);
            return ;
        }
    }
    else {
        Index::insertRecord(info, record_id, now->child[i]);
        Index::convert_BtreeNode_to_buf(now);
    }
}