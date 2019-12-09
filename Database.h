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
    int mem_file; // the lable of '.storage' 
    uint64_t mem; // mem_file's end offset

    Database(const char* name, bool create);
    ~Database();
    Sheet* createSheet(const char* name, int col_num, Type* col_ty);
    Sheet* openSheet(const char* name);
    void showSheets();
    int findSheet(std::string s);
    Sheet* findSheetPointer(std::string s);

    char* getVarchar(uint64_t idx); // get varchar from '.storage'
    uint64_t storeVarchar(char* str); // store varchar into '.storage'

};

#endif