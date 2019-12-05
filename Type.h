#ifndef __TYPE
#define __TYPE

#include "constants.h"
#include "Any.h"

#include "json.hpp"
using json = nlohmann::json;


enum enumType {
    INT,
    // TODO: FLOAT
    CHAR
    // TODO: VARCHAR
    // need to modify size(), [sheet.cpp]insert(), [sheet.cpp]get()
};

enum enumKeyType {
    Primary,
    Foreign,
    Common
};

struct Type {
private:
    bool unique = false;
    bool null = true;
    bool deleted = false;
    uint8_t char_len = 0; // only CHAR will use it
public:
    char name[MAX_NAME_LEN];
    // TODO: char comment[MAX_COMMENT_LEN];
    enumType ty = INT;
    enumKeyType key = Common;
    int foreign_sheet = -1; // TODO: when delete other sheet, need modify
    Any def; // TODO: maybe can be NULL, but now NULL means no-def

    bool isUnique() { return unique || key == Primary; }
    void setUnique(bool _unique) { unique = _unique; }
    bool isNull() { return null && key == Common; }
    void setNull(bool _null) { null = _null; }
    bool isDelete() { return deleted; }
    void del() { deleted = true; }
    
    uint size() {
        if (ty == INT) return 4;
        if (ty == CHAR) return char_len;
        return -1;
    }
    Type(const char* _name = "", enumType _ty = INT, uint8_t _char_len = 0, 
         enumKeyType _key = Common, int _foreign_sheet = 0, Any _def = Any(), bool _unique = false, bool _null = true) 
    : unique(_unique), null(_null), char_len(_char_len), ty(_ty), key(_key), foreign_sheet(_foreign_sheet), def(_def) {
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
            if (ty == enumType::INT) {
                def = j["def"].get<int>();
            } else if (ty == enumType::CHAR) {
                def = j["def"].get<std::string>().c_str();
            }
        } else {
            def = Any();
        }
        foreign_sheet = j["foreign_sheet"];
    }

    void setForeignKey(int sheet_id) { key = Foreign; foreign_sheet = sheet_id; }
    void unsetForeignKey() { key = Common; foreign_sheet = -1; }

};

#endif