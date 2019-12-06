#ifndef __TYPE
#define __TYPE

#include "constants.h"
#include "Any.h"

#include "json.hpp"
using json = nlohmann::json;


enum enumType {
    INT, // int
    CHAR, // char*
    VARCHAR, // char*
    DATA, // uint32_t
    DECIMAL, // long double
};

enum enumKeyType {
    Primary,
    Foreign,
    Common,
};

struct Type {
private:
    bool unique = false;
    bool null = true;
    bool deleted = false;
public:
    char name[MAX_NAME_LEN];
    // TODO: char comment[MAX_COMMENT_LEN];
    enumType ty = INT;
    uint8_t char_len = 0; // only CHAR will use it
    enumKeyType key = Common;
    int foreign_sheet = -1; // TODO: when delete other sheet, need modify
    Any def; // TODO: maybe can be NULL, but now NULL means no-def

    bool isUnique() { return unique || key == Primary; }
    void setUnique(bool _unique) { unique = _unique; }
    bool isNull() { return null && key != Primary; }
    void setNull(bool _null) { null = _null; }
    bool isDelete() { return deleted; }
    void del() { deleted = true; }
    
    uint size() {
        switch (ty) {
        case INT: return 4;
        case CHAR: return char_len;
        case VARCHAR: return 8;
        case DATA: return 4;
        case DECIMAL: return 16;
        default: return -1;
        }
    }
    Type(const char* _name = "", enumType _ty = INT, uint8_t _char_len = 0, 
         enumKeyType _key = Common, int _foreign_sheet = 0, Any _def = Any(), bool _unique = false, bool _null = true) 
    : unique(_unique), null(_null), ty(_ty), char_len(_char_len), key(_key), foreign_sheet(_foreign_sheet), def(_def) {
        strcpy(name, _name);
    }

    json toJson() {
        json j;
        j["name"] = name;
        j["ty"] = ty;
        j["char_len"] = char_len;
        j["key"] = key;
        j["unique"] = unique;
        j["null"] = null;
        if (def.anyCast<int>() != nullptr) {
            j["def"] = def.anyRefCast<int>();
        } else if (def.anyCast<char*>() != nullptr) {
            j["def"] = def.anyRefCast<char*>();
        }
        j["foreign_sheet"] = foreign_sheet;
        return j;
    }
    Type(json j) {
        strcpy(name, j["name"].get<std::string>().c_str());
        ty = (enumType)j["ty"].get<int>();
        char_len = j["char_len"].get<int>();
        key = (enumKeyType)j["key"].get<int>();
        unique = j["unique"].get<bool>();
        null = j["null"].get<bool>();
        if (j.count("def")) {
            switch (ty) {
            case INT: { def = j["def"].get<int>(); break; }
            case DATA: { def = j["def"].get<int>(); break; }
            case CHAR: { def = j["def"].get<std::string>().c_str(); break; }
            case VARCHAR: { def = j["def"].get<std::string>().c_str(); break; }
            case DECIMAL: { def = j["def"].get<long double>(); break; }
            }
        } else {
            def = Any();
        }
        foreign_sheet = j["foreign_sheet"];
    }

    void setForeignKey(int sheet_id) { key = Foreign; foreign_sheet = sheet_id; }
    void unsetForeignKey() { key = Common; foreign_sheet = -1; }

    int printLen() {
        switch (ty) {
        case INT: return 10;
        case CHAR: return std::min(char_len, (uint8_t)20);
        case VARCHAR: return std::min(char_len, (uint8_t)20);
        case DATA: return 10;
        case DECIMAL: return 10;
        default: return -1;
        }
    }
};

#endif