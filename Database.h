#ifndef __DATABASE
#define __DATABASE

#include "constants.h"
#include "Type.h"

#include "json.hpp"
using json = nlohmann::json;

typedef std::pair<std::string, std::string> Pss;
typedef std::pair<int, std::string> Pis;
typedef std::pair<int, uint> Piu;

class Sheet;
class FileManager;
class BufPageManager;

enum enumOp {
    OP_EQ,     // =
    OP_NEQ,    // !=
    OP_LE,     // <=
    OP_GE,     // >=
    OP_LS,     // <
    OP_GT,     // >
    OP_NL,     // is null
    OP_NNL,    // is not null
    OP_IN,     // in
};

struct WhereStmt {
    std::vector<Piu> cols;
    enumOp op;
    bool any = false;
    bool all = false;
    uint rvalue_ty = 0;
    Any rvalue;                                     // rvalue_ty = 1
    std::vector<Piu> rvalue_cols;  // rvalue_ty = 2
    int rvalue_sheet;                               // rvalue_ty = 3
};

struct SelectStmt {
    std::vector<Piu> select;
    std::vector<Pis> form; 
    std::vector<WhereStmt> where;
    std::vector<int> recursion;
};

class Database {
private:
    json toJson();
    void fromJson(json j);
    void update();
    
public:
    char name[MAX_NAME_LEN];
    Sheet* sheet[MAX_SHEET_NUM]; // need to modify foreign key
    uint sheet_num;

    FileManager* fm;
    BufPageManager* bpm;
    int mem_file; // the lable of '.storage' 
    uint64_t mem; // mem_file's end offset

    SelectStmt sel[MAX_SHEET_NUM];
    Sheet* sel_sheet[MAX_SHEET_NUM];
    uint sel_num = 0;

    Database(const char* name, bool create);
    ~Database();
    Sheet* createSheet(const char* name, int col_num, Type* col_ty);
    Sheet* openSheet(const char* name);
    void showSheets();
    int findSheet(std::string s);
    Sheet* findSheetPointer(std::string s);

    char* getVarchar(uint64_t idx); // get varchar from '.storage'
    uint64_t storeVarchar(char* str); // store varchar into '.storage'

    void querySel(uint idx);
};

#endif