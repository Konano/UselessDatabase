#ifndef __KEY
#define __KEY

#include "constants.h"

#include <math.h>
#include <vector>

#include "json.hpp"
using json = nlohmann::json;

class Sheet;
class Database;

class Key {
private:
    uint v_size = 0;
public:
    std::string name;
    std::vector<uint> v;
    Sheet* sheet;
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
    Key(Sheet* s) : sheet(s) {}
    Key(Sheet* s, json j) : sheet(s) {
        v_size = j.size();
        for (uint i = 0; i < v_size; i++) v.push_back(j[i].get<int>());
    }
    Key(Sheet* s, uint sz, int* info) : v_size(sz), sheet(s) { 
        for (uint i = 0; i < v_size; i++) v.push_back(info[i]); 
    }
    virtual ~Key() {}
    json toJson() {
        json j;
        for (uint i = 0; i < v_size; i++) j.push_back(v[i]);
        return j;
    }
};
class PrimaryKey;
class ForeignKey;

class PrimaryKey: public Key {
public:
    std::vector<ForeignKey*> f;
    PrimaryKey(Sheet* s) : Key(s) {}
    PrimaryKey(Sheet* s, json j) : Key(s, j) {}
    PrimaryKey(Sheet* s, uint sz, int* info) : Key(s, sz, info) {}
    int ty() { return 1; }
};

class ForeignKey: public Key {
public:
    PrimaryKey* p;
    ForeignKey(Sheet* s) : Key(s) {}
    ForeignKey(Sheet* s, json j, Database* db);
    ForeignKey(Sheet* s, uint sz, int* info) : Key(s, sz, info) {}
    int ty() { return 2; }
    json toJson();
};

#endif