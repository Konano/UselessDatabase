#ifndef __KEY
#define __KEY

#include "constants.h"

#include <math.h>
#include <vector>
#include <iostream>

#include "json.hpp"
using json = nlohmann::json;

class Table;
class Database;

class Key {
public:
    uint v_size = 0;
    std::string name;
    std::vector<uint> v;
    Table* table;
    virtual int ty() { return 0; }
    uint size() const { return v_size; }
    void push(int val) { v.push_back(val); v_size++; }
    void push_back(int val) { v.push_back(val); v_size++; }
    uint get(int idx) const { return v[idx]; }
    void sort() { std::sort(v.begin(), v.end()); }
    uint operator[](int idx) const { return v[idx]; }
    bool operator==(const Key& b) {
        if (v_size != b.size()) return false;
        for (uint i = 0; i < v_size; i++) {
            if (v[i] != b[i]) return false;
        }
        return true;
    }
    bool operator!=(const Key& b) { return !(*this == b); }
    Key(Table* s) : table(s) {}
    Key(Table* s, json j) : table(s) {
        name = j["name"].get<std::string>();
        v_size = j["cols"].size();
        for (uint i = 0; i < v_size; i++) v.push_back(j["cols"][i].get<int>());
    }
    Key(Table* s, uint sz, int* info) : v_size(sz), table(s) { 
        for (uint i = 0; i < v_size; i++) v.push_back(info[i]); 
    }
    virtual ~Key() {}
    json toJson() {
        json j;
        j["name"] = name.c_str();
        for (uint i = 0; i < v_size; i++) j["cols"].push_back(v[i]);
        return j;
    }
};
class PrimaryKey;
class ForeignKey;

class PrimaryKey: public Key {
public:
    std::vector<ForeignKey*> f;
    PrimaryKey(Table* s) : Key(s) {}
    PrimaryKey(Table* s, json j) : Key(s, j) {}
    PrimaryKey(Table* s, uint sz, int* info) : Key(s, sz, info) {}
    int ty() { return 1; }
};

class ForeignKey: public Key {
public:
    PrimaryKey* p;
    ForeignKey(Table* s) : Key(s) {}
    ForeignKey(Table* s, json j, Database* db);
    ForeignKey(Table* s, uint sz, int* info) : Key(s, sz, info) {}
    int ty() { return 2; }
    json toJson();
};

#endif