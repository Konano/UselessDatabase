#ifndef __ANYS
#define __ANYS

#include "Any.h"
#include <vector>

class Anys {
private:
    std::vector<Any> v;
public:
    Anys() {}

    Anys(Any* data, uint size) { 
        for (uint i = 0; i < size; i++) v.push_back(data[i]);
    }

    uint size() const { return v.size(); }

    void push_back(Any a) { v.push_back(a); }

    Any& operator[](uint idx) { return v[idx]; }

    void concatenate(Anys b) {
        uint sz = b.size();
        for (uint i = 0; i < sz; i++) v.push_back(b[i]);
    }

    void clear() { v.clear(); }

    bool operator==(const Anys &b) const {
        uint sz = size();
        if (sz != b.size()) return false;
        for (uint i = 0; i < sz; i++) if (!(v[i] == b.v[i])) return false;
        return true;
    }

    bool operator<(const Anys &b) const {
        uint sz = size(), _sz = b.size();
        if (sz != _sz) return sz < _sz;
        for (uint i = 0; i < sz; i++) if (!(v[i] == b.v[i])) return v[i] < b.v[i];
        return false;
    }

    bool operator>(const Anys &b) const {
        uint sz = size(), _sz = b.size();
        if (sz != _sz) return sz > _sz;
        for (uint i = 0; i < sz; i++) if (!(v[i] == b.v[i])) return v[i] > b.v[i];
        return false;
    }

    bool operator!=(const Anys &b) const { return !(*this == b); }

    bool operator<=(const Anys &b) const { return !(*this > b); }

    bool operator>=(const Anys &b) const { return !(*this < b); }
};

#endif