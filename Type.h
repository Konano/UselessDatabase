#ifndef __TYPE
#define __TYPE

#include "constants.h"
#include "Any.h"

#include "json.hpp"
using json = nlohmann::json;

enum enumType {
    INT,
    // FLOAT,
    CHAR
    // VARCHAR
};

enum enumKeyType {
    Primary,
    Foreign,
    Common
};

struct Type {
    char name[MAX_NAME_LEN];
    // char comment[MAX_COMMENT_LEN];
    enumType ty = enumType::INT;
    uint8_t len = 0; // only CHAR will use it
    enumKeyType key = enumKeyType::Common;
    bool unique = false;
    bool null = true;
    Any def = nullptr; 
    uint size() {
        if (ty == enumType::INT) return 4;
        if (ty == enumType::CHAR) return len;
        // if (ty == enumType::CHAR) return type_len;
        // if (ty == enumType::FLOAT) return 4;
        return -1;
    }
    Type(const char* _name = "", enumType _ty = enumType::INT, uint8_t _len = 0) : ty(_ty) {
        memcpy(name, _name, strlen(_name));
        if (_ty == enumType::CHAR) {
            len = _len;
        }
    }

    json toJson() {
        json j;
        j["name"] = name;
        j["ty"] = ty;
        j["len"] = len;
        j["key"] = key;
        j["unique"] = unique;
        j["null"] = null;
        if (def.anyCast<int>() != nullptr) {
            j["def"] = def.anyRefCast<int>();
        } else if (def.anyCast<char*>() != nullptr) {
            j["def"] = def.anyRefCast<char*>();
        }
        return j;
    }
    void fromJson(json j) {
        // TODO:
    }

};

#endif