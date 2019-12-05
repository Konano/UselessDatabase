#ifndef __DATABASE
#define __DATABASE

#include "constants.h"
#include "Type.h"

#include "json.hpp"
using json = nlohmann::json;

class Sheet;
class FileManager;
class BufPageManager;

class Database {
private:
    json toJson();
    void fromJson(json j);
    void update();
    
public:
    char name[MAX_NAME_LEN];
    Sheet* sheet[MAX_SHEET_NUM]; // need to modify foreign key
    int sheet_num;

    FileManager* fm;
    BufPageManager* bpm;

    Database(const char* name, bool create);
    ~Database();
    Sheet* createSheet(const char* name, int col_num, Type* col_ty);
    Sheet* openSheet(const char* name);
    // void showSheet();

};

#endif