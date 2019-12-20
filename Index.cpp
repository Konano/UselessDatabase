#include "Index.h"

#include "BufPageManager.h"
#include "Sheet.h"
#include "FileManager.h"
#include "Database.h"

#include <iostream>
#include <stdio.h>
#include <math.h>
#include <vector>
using namespace std;

extern char* dirPath(const char* dir, const char* filename, const char* subname, const char* filetype);

json Index::toJson() {
    json j;
    j["name"] = name;
    j["key_num"] = key_num;
    for (uint i = 0; i < key_num; i++) j["key"].push_back(key[i]);
    for (uint i = 0; i < key_num; i++) j["offset"].push_back(offset[i]);
    j["page_num"] = page_num;
    j["root_page"] = root_page;
    j["next_del_page"] = next_del_page;
    j["record_size"] = record_size;
    for (uint i = 0; i < key_num; i++) j["ty"].push_back(ty[i]);
    return j;
}

Index::Index(Sheet* sheet, const char* name, vector<uint> key,int btree_max_per_node) : sheet(sheet), key(key), btree_max_per_node(btree_max_per_node) {
    strcpy(this->name, name);
    cout << sheet->name << endl;
    sheet->fm->createFile(dirPath(sheet->db->name, sheet->name, name, "usid"));
    page_num = 1;
    next_del_page = -1;
    record_size = 8;
    for(uint i = 0;i < key.size();i ++)
        record_size += sheet->col_ty[i].size();
    max_recond_num = (PAGE_SIZE - 18) / record_size;

    open();

    int index;
    BufType buf = sheet->bpm->getPage(fileID, 0, index);
    *(uint32_t *)(buf+0) = -1;
    *(uint32_t *)(buf+4) = -1;
    *(uint32_t *)(buf+8) = -1;
    *(uint32_t *)(buf+12) = -1;
    *(uint16_t *)(buf+16) = 0;
    sheet->bpm->markDirty(index);
    
    root_page = 0;
    root = convert_buf_to_BtreeNode(root_page);
    root->fa_index = -1;
    close();
}

Index::Index(Sheet* sheet, json j) : sheet(sheet) {
    strcpy(name, j["name"].get<std::string>().c_str());
    key.clear();
    ty.clear();
    offset.clear();
    key_num = j["key_num"].get<int>();
    for (uint i = 0; i < key_num; i++) key.push_back(j["key"].get<int>());
    for (uint i = 0; i < key_num; i++) offset.push_back(j["offset"].get<int>());
    page_num = j["page_num"].get<int>();
    root_page = j["root_page"].get<int>();
    next_del_page = j["next_del_page"].get<int>();
    record_size = j["record_size"].get<int>();
    for (uint i = 0; i < key_num; i++) ty.push_back((enumType)j["ty"].get<int>());
    fileID = -1; // file is closed
}

void Index::open() {
    sheet->fm->openFile(dirPath(sheet->db->name, sheet->name, name, "usid"), fileID);
}

void Index::close() {
    sheet->fm->closeFile(fileID);
    fileID = -1; // file is closed
}

void Index::remove() {
    if (fileID != -1) close();
    sheet->fm->deleteFile(dirPath(sheet->db->name, sheet->name, name, "usid"));
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

void Index::Debug(){
    cout << "root " << " rootindex: " << root->index << " root_page: " << root_page << endl;
    debug(root);
}

void Index::debug(BtreeNode* node){
    cout << "node " << node->index << " fa " << node->fa_index << " isleaf " << node->is_leaf <<endl;
    cout << node->record_cnt << endl;
    printf("val:\n");
    for(auto a: node->record){
        cout << a.record_id << endl;
    }
    printf("son:\n");
    for(auto a: node->child){
        cout << a << endl;
    }
    for(auto a: node->child){
        if(a >= 0)debug(Index::convert_buf_to_BtreeNode(a));
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
    ans->fa_index = *(uint32_t *)(buf + 12);
    ans->record_cnt = *(uint16_t *)(buf + 16);

    if(ans->record_cnt != 0){ans->child.push_back(*(uint32_t *)(buf + 18)); if(ans->child[0] != -1)ans->is_leaf = false;}
    else ans->child.push_back(-1);

    for (int i = 0;i < ans->record_cnt;i ++){
        BtreeRecord temp;
        temp.record_id = *(uint32_t *)(buf + 22 + i * record_size);
        for (uint j = 0;j < this->key_num;j ++){
            if (this->ty[j] == enumType::INT) {
                temp.key[j] = *(int*)(buf + 22 + i * record_size + 4 + this->offset[j]);
            }
            if (this->ty[j] == enumType::CHAR) {
                char* str = new char[record_size - 8 + 1];
                memcpy(str, buf + 22 + i * record_size + 4 + this->offset[j], this->offset[j] - (j == 0 ? 0 : this->offset[j - 1]));
                str[record_size - 8] = '\0';
                temp.key[j] = str;
            }
        }
        ans->child.push_back(*(uint32_t *)(buf + 22 + (i + 1) * record_size - 4));
        ans->record.push_back(temp);
    }
    return ans;
}

void Index::convert_BtreeNode_to_buf(BtreeNode* node) {
    int _index;
    BufType buf = sheet->bpm->getPage(fileID, node->index, _index);

    *(uint32_t*) buf = -1; buf += 4; 
    *(uint32_t*) buf = node->left_page_index; buf += 4; 
    *(uint32_t*) buf = node->right_page_index; buf += 4; 
    *(uint32_t*) buf = node->fa_index; buf += 4;
    *(uint16_t*) buf = node->record_cnt; buf += 2; 

    if(node->record_cnt != 0){
        *(uint32_t*) buf = node->child[0]; buf += 4;
    }
    for (int i = 0;i < node->record_cnt;i ++){
        *(uint32_t*) buf = node->record[i].record_id; buf += 4;
        for (uint j = 0;j < this->key_num;j ++){
            if (this->ty[j] == enumType::INT) {
                *(uint32_t*)buf = *node->record[i].key[j].anyCast<int>();
                buf += 4;
            }
            if (this->ty[j] == enumType::CHAR) {
                memcpy(buf, *node->record[i].key[j].anyCast<char*>(), this->offset[j] - (j == 0 ? 0 : this->offset[j - 1]));
                buf += this->offset[j] - (j == 0 ? 0 : this->offset[j - 1]);
            }
        }
        *(uint32_t*) buf = node->child[i + 1]; buf += 4;
    }
    sheet->bpm->markDirty(_index);
}

std::vector<int> Index::queryRecord(Anys* info) {
    return Index::queryRecord(info, root_page);
}

std::vector<int> Index::queryRecord(Anys* info, int index) {
    
    BtreeNode *now = Index::convert_buf_to_BtreeNode(index);

    std::vector<int> v;

    int l = -1, r = -1, i;
    for (i = 0;i < now->record_cnt;i ++){
        if(now->record[i].key == *info){
            if (l == -1) l = i;
            r = i;
        }
        else if(now->record[i].key > *info){
            break;
        }
    }

    if (l == -1 && now -> is_leaf) return v;
    else if (l == -1){
        Index::convert_BtreeNode_to_buf(now);
        return Index::queryRecord(info, now->child[i]);
    }

    for(int i = l;i <= r;i ++)v.push_back(now -> record[i].record_id);

    if (now -> is_leaf){
        return v;
    }
    else {
        for (int i = l;i <= r + 1;i ++){
            Index::convert_BtreeNode_to_buf(now);
            std::vector<int> cv = Index::queryRecord(info, now->child[i]);
            for(uint j = 0;j < cv.size();j ++)v.push_back(cv[j]);
        }
        return v;
    }
}

void Index::overflow_downstream(BtreeNode* now){
    //cout << "down" << now->index << " "  << now->record_cnt << endl;
    if (now->fa_index == -1) {
        if(now->record_cnt != 0)return ;
        else {
            if(now->child[0] == -1)return ;
            root = Index::convert_buf_to_BtreeNode(now->child[0]);
            root_page = now->child[0];
            root->fa_index = -1;
            return ;
        }
    }
    else if((int)now->record.size() >= ceil(btree_max_per_node / 2.0) - 1){
        return ;
    }
    else {
        BtreeNode* fa = Index::convert_buf_to_BtreeNode(now->fa_index);
        uint i;
        for (i = 0;i < fa->child.size();i ++){
            if(fa->child[i] == now->index){
                break;
            }
        }
        if (i != 0){
            BtreeNode* left = Index::convert_buf_to_BtreeNode(fa->child[i - 1]);
            if (left->record_cnt <= ceil(btree_max_per_node / 2.0) - 1){
                //TODO: REUSE DELETED PAGE
                left->record.push_back(fa->record[i - 1]);
                for(int j = 0; j < now->record_cnt ;j ++){
                    left->record.push_back(now->record[j]);
                    left->child.push_back(now->child[j]);
                }
                left->record_cnt += 1 + now->record_cnt;
                left->child.push_back(now->child[now->child.size() - 1]);
                vector<BtreeRecord>::iterator it = fa->record.begin();
                vector<int>::iterator itx = fa->child.begin();
                itx++;
                uint temp = i;
                while(--temp){it ++; itx++;}
                fa->record.erase(it);
                fa->child.erase(itx);
                fa->record_cnt--;
                Index::convert_BtreeNode_to_buf(now);
                Index::convert_BtreeNode_to_buf(left);

                overflow_downstream(fa);        
                Index::convert_BtreeNode_to_buf(fa);
            }
            else {
                now->record.insert(now->record.begin(),fa->record[i - 1]);
                now->child.insert(now->child.begin(),-1);
                now->record_cnt ++;

                fa->record[i - 1] = left->record[left->record.size() - 1];

                left->record_cnt --;
                left->record.erase(left->record.end() - 1);
                left->child.erase(left->child.end() - 1);
                Index::convert_BtreeNode_to_buf(now);
                Index::convert_BtreeNode_to_buf(left);
                Index::convert_BtreeNode_to_buf(fa);
            }
        }
        else {
            BtreeNode* right = Index::convert_buf_to_BtreeNode(fa->child[i + 1]);
            if (right->record_cnt <= ceil(btree_max_per_node / 2.0) - 1){
                //TODO: REUSE DELETED PAGE
                now->record.push_back(fa->record[i]);
                for(int j = 0; j < right->record_cnt ;j ++){
                    now->record.push_back(right->record[j]);
                    now->child.push_back(right->child[j]);
                }
                now->record_cnt += 1 + right->record_cnt;
                now->child.push_back(right->child[right->child.size() - 1]);
                vector<BtreeRecord>::iterator it = fa->record.begin();
                vector<int>::iterator itx = fa->child.begin();
                uint temp = i;
                itx ++;
                while(temp--){it ++; itx++;}
                fa->record.erase(it);
                fa->child.erase(itx);
                fa->record_cnt--;
                Index::convert_BtreeNode_to_buf(now);
                Index::convert_BtreeNode_to_buf(right);

                overflow_downstream(fa);        
                Index::convert_BtreeNode_to_buf(fa);
            }
            else {
                now->record.insert(now->record.begin(),fa->record[i - 1]);
                now->child.insert(now->child.begin(),-1);
                now->record_cnt ++;

                fa->record[i - 1] = right->record[0];

                right->record_cnt --;
                right->record.erase(right->record.begin());
                right->child.erase(right->child.begin());
                Index::convert_BtreeNode_to_buf(now);
                Index::convert_BtreeNode_to_buf(right);
                Index::convert_BtreeNode_to_buf(fa);
            }
        }
    }
}

void Index::overflow_upstream(BtreeNode* now){

    //cout << "up" << now->index << " "  << now->record_cnt << endl;
    if ((int)now->record.size() <= btree_max_per_node - 1){
        return ;
    }
    else {

        BtreeNode* fa;

        if(now->fa_index == -1){
            fa = new BtreeNode();
            fa->index = page_num++;
            root = fa;
            root_page = page_num - 1;
            fa->is_leaf = false;
        }
        else{
            fa = Index::convert_buf_to_BtreeNode(now->fa_index);
        }
        BtreeNode* right = new BtreeNode();
        //TODO: Reuse del page
        right->index = page_num++;

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
        fa->record_cnt ++;

        Index::convert_BtreeNode_to_buf(now);
        Index::convert_BtreeNode_to_buf(right);

        overflow_upstream(fa);        
        Index::convert_BtreeNode_to_buf(fa);
    }
}

void Index::insertRecord(Anys* info, int record_id) {
    return insertRecord(info, record_id, root);
}

void Index::insertRecord(Anys* info, int record_id, BtreeNode* now) {

    //BtreeNode *now = Index::convert_buf_to_BtreeNode(index);
    //cout << "insert" << now->index << " "  << now->record_cnt ;
    //cout << " " << *info->anyCast<int>() << " " << record_id << endl;
    int i = 0;
    vector<BtreeRecord>::iterator it;
    for (it = now->record.begin();it!=now->record.end();it++)
    {
        if(it->key > *info){
            break;
        }
        if(it->key == *info && it->record_id > record_id){
            break;
        }
        i ++;
    }

    if (now->is_leaf){
        now->record.insert(it, BtreeRecord(record_id,*info));
        now->record_cnt++;
        now->child.push_back(-1);
        Index::convert_BtreeNode_to_buf(now);
        overflow_upstream(now);
        return ;
    }
    else {
        BtreeNode* child_i = Index::convert_buf_to_BtreeNode(now->child[i]);
        Index::insertRecord(info, record_id, child_i);
        Index::convert_BtreeNode_to_buf(now);
    }
}

void Index::removeRecord(Anys* info, int record_id){
    removeRecord(info,record_id,root);
}

void Index::removeRecord(Anys* info, int record_id, BtreeNode* now){
    
    //cout << "remove" << now->index << " "  << now->record_cnt << " " << record_id << endl;
    int i = 0;
    vector<BtreeRecord>::iterator it;
    for (it = now->record.begin();it!=now->record.end();it++)
    {
        if(it->key > *info){
            break;
        }
        if(it->key == *info && it->record_id >= record_id){
            break;
        }
        i ++;
    }
    if(it->record_id == record_id){
        if(now->is_leaf){
            now->record.erase(it);
            now->child.pop_back();
            now->record_cnt--;
            overflow_downstream(now);
            Index::convert_BtreeNode_to_buf(now);
        }
        else {
            BtreeNode* follow_node = Index::convert_buf_to_BtreeNode(now->child[i + 1]);
            while(follow_node->child[0] != -1)follow_node = Index::convert_buf_to_BtreeNode(follow_node->child[0]);
            now->record[i] = follow_node->record[0];
            Index::removeRecord(&follow_node->record[0].key, follow_node->record[0].record_id, follow_node);
            Index::convert_BtreeNode_to_buf(now);
            Index::convert_BtreeNode_to_buf(follow_node);
        }
    }
    else {
        if (!now->is_leaf) Index::removeRecord(info, record_id, Index::convert_buf_to_BtreeNode(now->child[i]));
        Index::convert_BtreeNode_to_buf(now);
    }
}
