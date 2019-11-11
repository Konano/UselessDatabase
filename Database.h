#ifndef __DATABASE
#define __DATABASE

#include "constants.h"
#include "Sheet.h"
#include "FileManager.h"
#include "BufPageManager.h"
#include "Type.h"

#include "json.hpp"
using json = nlohmann::json;

class Sheet;

class Database {

private:
    json toJson();
    void fromJson(json j);
    void update();
    
public:
    char name[MAX_NAME_LEN];
    Sheet* sheet[MAX_SHEET_NUM];
    int sheet_num;

    FileManager* fm;
    BufPageManager* bpm;

    Database(const char* name, bool create);
    ~Database();
    Sheet* createSheet(const char* name, int col_num, Type* col_ty);

};

#endif