#ifndef __TYPE
#define __TYPE

#include "constants.h"
#include "Any.h"

#include "json.hpp"
using json = nlohmann::json;

enum enumType {
    INT,     // int
    CHAR,    // char*
    VARCHAR, // char*
    DATE,    // uint32_t
    DECIMAL, // long double
};

enum enumKeyType {
    Primary,
    Foreign,
    Common,
};

struct Type {
private:
    bool null = true;
    bool deleted = false;
public:
    char name[MAX_NAME_LEN];
    enumType ty = INT;
    uint8_t char_len = 0; // only CHAR will use it
    enumKeyType key = Common;
    Any def;

    bool beNull() { return null; }
    void setNull(bool _null) { null = _null; }
    bool isDelete() { return deleted; }
    void del() { deleted = true; }
    
    uint size() {
        switch (ty) {
        case INT: return 4;
        case CHAR: return char_len;
        case VARCHAR: return 8;
        case DATE: return 4;
        case DECIMAL: return 16;
        default: return -1;
        }
    }
    Type(const char* _name = "", enumType _ty = INT, uint8_t _char_len = 0, 
         Any _def = Any(), bool _null = true) 
    : null(_null), ty(_ty), char_len(_char_len), def(_def) {
        strcpy(name, _name);
    }

    json toJson() {
        json j;
        j["name"] = name;
        j["ty"] = ty;
        j["char_len"] = char_len;
        j["key"] = key;
        j["null"] = null;
        if (def.anyCast<int>() != nullptr) {
            j["def"] = def.anyRefCast<int>();
        } else if (def.anyCast<char*>() != nullptr) {
            j["def"] = def.anyRefCast<char*>();
        }
        return j;
    }
    Type(json j) {
        strcpy(name, j["name"].get<std::string>().c_str());
        ty = (enumType)j["ty"].get<int>();
        char_len = j["char_len"].get<int>();
        key = (enumKeyType)j["key"].get<int>();
        null = j["null"].get<bool>();
        if (j.count("def")) {
            switch (ty) {
            case INT: { def = j["def"].get<int>(); break; }
            case DATE: { def = j["def"].get<int>(); break; }
            case CHAR: { def = j["def"].get<std::string>().c_str(); break; }
            case VARCHAR: { def = j["def"].get<std::string>().c_str(); break; }
            case DECIMAL: { def = j["def"].get<long double>(); break; }
            }
        } else {
            def = Any();
        }
    }

    int printLen() {
        switch (ty) {
        case INT: return 10;
        case CHAR: return std::min(char_len, (uint8_t)20);
        case VARCHAR: return std::min(char_len, (uint8_t)20);
        case DATE: return 10;
        case DECIMAL: return 10;
        default: return -1;
        }
    }
};

#endif