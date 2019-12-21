%{
#include "parser.h"
#include "Sheet.h"
#include "Database.h"
#include "Type.h"

extern int cleanDatabase(const char *dbname);
extern char* dirPath(const char* dir, const char* path);
extern int checkFile(const char *filename);
extern void showDatabases();
extern string Type2Str(enumType ty);
extern string Any2Str(Any val);

extern "C"
{
    void yyerror(const char *s);
    extern int yylex(void);
};

extern Database* db;
bool error;

#define filterSheet(it) ((it) < 0 ? db->sel_sheet[-1-it] : db->sheet[it])
#define Piu2Col(it) (filterSheet((it).first)->col_ty[(it).second])

uint month[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

bool current_db_exists(){
    if(db == nullptr)printf("Select a database first\n");
    return db != nullptr;
}

bool table_exists(std::string name, int& tableID, bool expect){
    tableID = db->findSheet(name);
    if(tableID == -1 && expect)printf("TABLE %s doesn't exist\n", name.c_str());
    if(tableID != -1 && !expect)printf("TABLE %s exists\n", name.c_str());
    return tableID != -1;
}

vector<Piu> dealCol(vector<Pss> cols, vector<Pis> from) {
	vector<Piu> v; 
	for (auto it: cols) {
        bool findTable = false;
        if (it.first == string() && from.size() > 1) {
            printf("COLUMN %s need to assign TABLE\n", it.second.c_str());
            error = true;
            continue;
        }
		for (auto _it: from) if ((it.first == _it.second) || (it.first == string() && from.size() == 1)) {
            findTable = true;
			int col = filterSheet(_it.first)->findCol(it.second);
            if (col < 0) {
                printf("COLUMN %s doesn't exist in TABLE %s\n", it.second.c_str(), _it.second.c_str());
                error = true;
                continue;
            }
			v.push_back(make_pair(_it.first, (uint)col));
			break;
		}
        
        if (findTable == false) { 
            printf("TABLE %s doesn't exist\n", it.first.c_str());
            error = true;
            continue;
        }
	}
	return v;
}

void dealTy(uint idx) {
	if (db->sel[idx].select.size() == 0) {
        for (auto it: db->sel[idx].from) {
            Sheet* st = filterSheet(it.first);
            for (uint i = 0; i < st->col_num; i++) {
                db->sel_sheet[idx]->col_ty[db->sel_sheet[idx]->col_num++] = st->col_ty[i];
            }
        }
    } else {
        for (auto it: db->sel[idx].select) {
            db->sel_sheet[idx]->col_ty[db->sel_sheet[idx]->col_num++] = filterSheet(it.first)->col_ty[it.second];
        }
    }
}

bool tycheck_any(Type ty, Any v) {
    switch (ty.ty) {
        case INT: 
            return v.anyCast<int>() != nullptr;
        case CHAR: 
            return (v.anyCast<char*>() != nullptr) && (strlen((const char*)v.anyCast<char*>()) == ty.char_len);
        case VARCHAR: 
            return (v.anyCast<char*>() != nullptr) && (strlen((const char*)v.anyCast<char*>()) <= ty.char_len);
        case DATE: 
            return v.anyCast<uint32_t>() != nullptr;
        case DECIMAL: 
            return v.anyCast<long double>() != nullptr;
        default:
            return false;
    }
};

bool tycheck_ty(Type ty, Type _ty) {
    if (ty.ty == _ty.ty) return true;
    if (ty.ty == CHAR && _ty.ty == VARCHAR) return true;
    if (ty.ty == VARCHAR && _ty.ty == CHAR) return true;
    return false;
};

const char *op2str(enumOp op) {
    switch (op) {
    case OP_EQ: return "=";   
    case OP_NEQ: return "<>";
    case OP_LE: return "<=";
    case OP_GE: return ">=";    
    case OP_LS: return "<";    
    case OP_GT: return ">";    
    case OP_NL: return "IS NULL";   
    case OP_NNL: return "IS NOT NULL";   
    case OP_IN: return "IN";
    default: return "";
    }
}

%}

%token DATABASE
%token DATABASES
%token TABLE
%token TABLES
%token SHOW
%token CREATE
%token DROP
%token USE
%token PRIMARY
%token KEY
%token NOT
%token NL
%token INSERT
%token INTO
%token VALUES
%token DELETE
%token FROM
%token WHERE
%token UPDATE
%token SET
%token SELECT
%token IS
%token TYPE_INT
%token TYPE_VARCHAR
%token TYPE_CHAR
%token DEFAULT
%token CONSTRAINT
%token CHANGE
%token ALTER
%token ADD
%token RENAME
%token DESC
%token REFERENCES
%token INDEX
%token AND
%token TYPE_DATE
%token TYPE_FLOAT
%token FOREIGN
%token EXIT
%token ON
%token TO
%token LB RB
%token SEMI
%token COMMA
%token STAR
%token EQ NEQ LE GE LS GT
%token DOT
%token IN ANY ALL
%token AS
%token COUNT MIN MAX AVG SUM
%token <S> IDENTIFIER
%token <I> VALUE_INT
%token <S> VALUE_STRING
%token <U> VALUE_DATE
%token <D> VALUE_LONG_DOUBLE
%type <S> dbName tbName colName pkName fkName idxName aliasName
%type <TY> type
%type <TY> field
%type <V_TY> fields
%type <V> value
%type <V_V> values _values
%type <P_SS> col
%type <V_P_SS> selector cols _cols
%type <I> seleStmt
%type <P_IS> fromClause
%type <V_P_IS> fromClauses
%type <W> whereClause
%type <V_W> whereClauses
%type <eOP> op
%type <V_S> colNames
%type <V_P_SA> setClause
%type <P__P_ISP_SS> aggregation
%type <V_P__P_ISP_SS> aggregations _aggregations

%%

program:
    | EXIT {
        return 0;
    }
    | stmt program
    ;

stmt:
    error
    | sysStmt
    | dbStmt
    | tbStmt
    | idxStmt
    | alterStmt;

sysStmt:
    SHOW DATABASES SEMI {
        showDatabases();
    };

dbStmt:
    CREATE DATABASE dbName SEMI {
        if (checkFile(dirPath($3.c_str(), ".storage")) == 0) {
            new Database($3.c_str(), true);   
        } else {
            printf("DATABASE %s exists\n",$3.c_str());
        }
    }
    | DROP DATABASE dbName SEMI {
        if (checkFile(dirPath($3.c_str(), ".storage")) == 1) {
            if (db && strcmp(db->name, $3.c_str()) == 0) {
                delete db;
                db = nullptr;
            }
            cleanDatabase($3.c_str());
        } else {
            printf("DATABASE %s doesn't exist\n",$3.c_str());
        }
    }
    | USE dbName SEMI {
        if (checkFile(dirPath($2.c_str(), ".storage")) == 1) {
            if (db) delete db;
            db = new Database($2.c_str(), false);
        } else {
            printf("DATABASE %s doesn't exist\n",$2.c_str());
        }
    }
    | SHOW TABLES SEMI {
        if (current_db_exists()) {
            db->showSheets();
        }
    };

tbStmt:
    CREATE TABLE tbName LB fields RB SEMI {
        if (current_db_exists()) {
            int tableID;
            if(!table_exists($3,tableID,false)){
                Type* ty = new Type[$5.size()];
                bool flag = true;
                for (uint i = 0; i < $5.size(); i++) {
                    ty[i] = $5[i];
                    for (uint j = 0; j < i; j++) {
                        if (std::string(ty[i].name) == std::string(ty[j].name)) {
                            flag = false;
                            break;
                        }
                    }
                    if (!flag) break;
                }
                if (flag) {
                    db->createSheet($3.c_str(), $5.size(), ty);
                    db->update();
                }
                else {
                    printf("Column name conflict\n");
                }
            }
        }
    }
    | DROP TABLE tbName SEMI {
        if (current_db_exists()) {
            if (db->deleteSheet($3.c_str())) printf("TABLE %s doesn't exist\n", $3.c_str());
        }
    }
    | DESC tbName SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($2,tableID,true)) {
                db->sheet[tableID]->printCol();
            }
        }
    }
    | INSERT INTO tbName VALUES LB values RB SEMI{
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){

            }
        }
    }
    | INSERT INTO tbName LB colNames RB VALUES LB values RB SEMI{
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                
            }
        }
    }
    | DELETE FROM tbName WHERE whereClauses SEMI
    | UPDATE tbName SET setClause WHERE whereClauses SEMI
    | seleStmt SEMI {
        if(current_db_exists()){
            db->buildSel(-1 - $1);
            db->sel_sheet[-1 - $1]->print();
            while (db->sel_num) delete db->sel_sheet[--(db->sel_num)];
        }
    };

seleStmt:
    SELECT selector FROM fromClauses {
        if(current_db_exists()){
            for (auto it: $4) if (it.first == MAX_SHEET_NUM) YYERROR;
            error = false;
            uint idx = db->sel_num++;
            db->sel_sheet[idx] = new Sheet(1);
            db->sel[idx].select = dealCol($2, $4);
            db->sel[idx].from = $4;
            db->sel[idx].recursion.clear();
            for (auto it: $4) if (it.first < 0) {
                db->sel[idx].recursion.push_back(it.first);
            }
            db->sel[idx].where.clear();
            dealTy(idx);
            $$ = -1 - idx;
            
            if (error) {
                delete db->sel_sheet[--db->sel_num];
                YYERROR;
            }
        }
        else {YYERROR;}
    }
    | SELECT _aggregations FROM fromClauses {
       if(current_db_exists()){
            for (auto it: $4) if (it.first == MAX_SHEET_NUM) YYERROR;
            error = false;
            uint idx = db->sel_num++;
            db->sel_sheet[idx] = new Sheet(2);
            // db->sel[idx].select = dealCol($2, $4);
            db->sel[idx].from = $4;
            db->sel[idx].recursion.clear();
            for (auto it: $4) if (it.first < 0) {
                db->sel[idx].recursion.push_back(it.first);
            }
            db->sel[idx].where.clear();
            // dealTy(idx);
            $$ = -1 - idx;
            
            if (error) {
                delete db->sel_sheet[--db->sel_num];
                YYERROR;
            }
        }
        else {YYERROR;}
    }
    | SELECT selector FROM fromClauses WHERE whereClauses {
        if(current_db_exists()){
            for (auto it: $4) if (it.first == MAX_SHEET_NUM) YYERROR;
            error = false;
            uint idx = db->sel_num++;
            db->sel_sheet[idx] = new Sheet(1);
            db->sel[idx].select = dealCol($2, $4);
            db->sel[idx].from = $4;
            db->sel[idx].recursion.clear();
            for (auto it: $4) if (it.first < 0) {
                db->sel[idx].recursion.push_back(it.first);
            }
            db->sel[idx].where.clear();
            for (auto it: $6) {
                WhereStmt where;
                where.cols = dealCol(it.cols, $4);
                where.op = it.op;
                where.rvalue = it.rvalue;
                where.rvalue_cols = dealCol(it.rvalue_cols, $4);
                where.rvalue_sheet = it.rvalue_sheet;
                where.any = (it.ty == 5);
                where.all = (it.ty == 6);
                switch (it.ty) {
                case 0:
                    where.rvalue_ty = 1;
                    if (where.cols.size() != where.rvalue.size()) {
                        printf("length is not equal\n");
                        break;
                    }
                    for (uint i = 0, size = where.cols.size(); i < size; i++) {
                        if (tycheck_any(Piu2Col(where.cols[i]), where.rvalue[i]) == false) {
                            printf("Type error: %s.%s(%s) %s %s\n", 
                                filterSheet(where.cols[i].first)->name, 
                                Piu2Col(where.cols[i]).name, 
                                Type2Str(Piu2Col(where.cols[i]).ty).c_str(),
                                op2str(where.op), 
                                Any2Str(where.rvalue[i]).c_str());
                            error = true;
                        }
                    }
                    break;
                case 1:
                    where.rvalue_ty = 2;
                    if (where.cols.size() != where.rvalue_cols.size()) {
                        printf("length is not equal\n");
                        break;
                    }
                    for (uint i = 0, size = where.cols.size(); i < size; i++) {
                        if (tycheck_ty(Piu2Col(where.cols[i]), Piu2Col(where.rvalue_cols[i])) == false) {
                            printf("Type error: %s.%s(%s) %s %s.%s(%s)\n", 
                                filterSheet(where.cols[i].first)->name, 
                                Piu2Col(where.cols[i]).name, 
                                Type2Str(Piu2Col(where.cols[i]).ty).c_str(),
                                op2str(where.op), 
                                filterSheet(where.rvalue_cols[i].first)->name, 
                                Piu2Col(where.rvalue_cols[i]).name,
                                Type2Str(Piu2Col(where.rvalue_cols[i]).ty).c_str());
                            error = true;
                        }
                    }
                    break;
                case 2:
                case 3:
                    where.rvalue_ty = 0;
                    break;
                case 4:
                case 5:
                case 6:
                case 7:
                    where.rvalue_ty = 3;
                    if (where.cols.size() != filterSheet(where.rvalue_sheet)->col_num) {
                        printf("length is not equal\n");
                        break;
                    }
                    for (uint i = 0, size = where.cols.size(); i < size; i++) {
                        if (tycheck_ty(Piu2Col(where.cols[i]), filterSheet(where.rvalue_sheet)->col_ty[i]) == false) {
                            printf("Type error: %s.%s(%s) %s %s.%s(%s)\n", 
                                filterSheet(where.cols[i].first)->name, 
                                Piu2Col(where.cols[i]).name, 
                                Type2Str(Piu2Col(where.cols[i]).ty).c_str(),
                                op2str(where.op), 
                                filterSheet(where.rvalue_sheet)->name, 
                                filterSheet(where.rvalue_sheet)->col_ty[i].name,
                                Type2Str(filterSheet(where.rvalue_sheet)->col_ty[i].ty).c_str());
                            error = true;
                        }
                    }
                    break;
                }
                if (where.rvalue_sheet < 0) {
                    db->sel[idx].recursion.push_back(where.rvalue_sheet);
                }
                db->sel[idx].where.push_back(where);
            }
            dealTy(idx);
            $$ = -1 - idx;
            
            if (error) {
                delete db->sel_sheet[--db->sel_num];
                YYERROR;
            }
        }
        else{YYERROR;}
    };

idxStmt:
    CREATE INDEX idxName ON tbName LB colNames RB SEMI {
        if(current_db_exists()){
            int tableID;
            if(table_exists($5,tableID,true)){
                if (db->sheet[tableID]->findIndex($3) != -1) printf("INDEX %s exists\n", $3.c_str());
                else{
                    vector<uint> key_index;
                    bool create_ok = true;
                    for(uint i = 0;i < $7.size();i ++){
                        bool flag = false;
                        for(uint j = 0;j < db->sheet[tableID]->col_num;j ++){
                            if ($7[i] == std::string(db->sheet[tableID]->col_ty[j].name)){
                                flag = true;
                                key_index.push_back(j);
                                break;
                            }
                        }
                        if (!flag) {
                            printf("TABLE %s doesn't have a column named %s\n",$5.c_str(),$7[i].c_str());
                            create_ok = false;
                            break;
                        }
                    }
                    if(create_ok){
                        db->sheet[tableID]->createIndex(key_index,$3);
                    }
                }
            }
            db->update();
        }
    }
    | DROP INDEX idxName SEMI {
        if(current_db_exists()){
            bool flag = false;
            for(uint i = 0;i < db->sheet_num;i ++){
                int indexID = db->sheet[i]->findIndex($3);
                if(indexID != -1){
                    db->sheet[i]->removeIndex(indexID);
                    flag = true;
                    break;
                }
            }
            if (!flag) printf("INDEX %s doesn't exist\n", $3.c_str());
            db->update();
        }
    }
    | ALTER TABLE tbName ADD INDEX idxName LB colNames RB SEMI {
        //TODO: 这个和第一个CREATE的区别
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                if (db->sheet[tableID]->findIndex($6) != -1) printf("INDEX %s exists\n", $6.c_str());
                else{
                    vector<uint> key_index;
                    bool create_ok = true;
                    for(uint i = 0;i < $8.size();i ++){
                        bool flag = false;
                        for(uint j = 0;j < db->sheet[tableID]->col_num;j ++){
                            if ($8[i] == std::string(db->sheet[tableID]->col_ty[j].name)){
                                flag = true;
                                key_index.push_back(j);
                                break;
                            }
                        }
                        if (!flag) {
                            printf("TABLE %s doesn't have a column named %s\n",$3.c_str(),$8[i].c_str());
                            create_ok = false;
                            break;
                        }
                    }
                    if(create_ok){
                        db->sheet[tableID]->createIndex(key_index,$6);
                    }
                }
            }
            db->update();
        }
    }
    | ALTER TABLE tbName DROP INDEX idxName SEMI{
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                int indexID = db->sheet[tableID]->findIndex($6);
                if(indexID != -1){
                    db->sheet[tableID]->removeIndex(indexID);
                }
                else printf("INDEX %s doesn't exist\n", $6.c_str());
            }
            db->update();
        }
    };

alterStmt:
    ALTER TABLE tbName ADD field SEMI{
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                if(db->sheet[tableID]->findCol(std::string($5.name)) != -1){
                    printf("Column %s is used\n",$5.name);
                }
                else {
                    db->sheet[tableID]->createColumn($5);
                }
            }
            db->update();
        }
    }
    | ALTER TABLE tbName DROP colName SEMI {
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                if(db->sheet[tableID]->findCol($5) != -1)db->sheet[tableID]->removeColumn(db->sheet[tableID]->findCol($5));
                else printf("Column %s doesn't exist\n",$5.c_str());
            }
            db->update();
        }
    }
    | ALTER TABLE tbName CHANGE colName field SEMI {
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                if(db->sheet[tableID]->findCol($5) != -1){
                    if(db->sheet[tableID]->findCol(std::string($6.name)) == -1 || $5 == std::string($6.name))db->sheet[tableID]->modifyColumn(db->sheet[tableID]->findCol($5), $6);
                    else printf("Column %s is used\n",$6.name);
                }
                else printf("Column %s doesn't exist\n",$5.c_str());
            }
            db->update();
        }
    }
    | ALTER TABLE tbName RENAME TO tbName SEMI {
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                for(uint i = 0;i < $6.length();i ++){
                    db->sheet[tableID]->name[i] = $6[i];
                }
                db->sheet[tableID]->name[$6.length()] = '\0';
            }
            db->update();
        }
    }
    | ALTER TABLE tbName ADD PRIMARY KEY LB colNames RB SEMI {
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                if(db->sheet[tableID]->p_key != nullptr)printf("TABLE %s has a primary key\n",$3.c_str());
                else {
                    int* key_index = new int[$8.size()];
                    bool create_ok = true;
                    for(uint i = 0;i < $8.size();i ++){
                        bool flag = false;
                        for(uint j = 0;j < db->sheet[tableID]->col_num;j ++){
                            if ($8[i] == std::string(db->sheet[tableID]->col_ty[j].name)){
                                flag = true;
                                key_index[i] = j;
                                break;
                            }
                        }
                        if (!flag) {
                            printf("TABLE %s doesn't have a column named %s\n",$3.c_str(),$8[i].c_str());
                            create_ok = false;
                            break;
                        }
                    }
                    if(create_ok){
                        PrimaryKey temp = PrimaryKey(db->sheet[tableID], $8.size(), key_index);
                        int x = db->sheet[tableID]->createPrimaryKey(&temp);
                        if (x == -2)printf("Constraint problem\n");
                    }
                }
            }
            db->update();
        }
    }
    | ALTER TABLE tbName DROP PRIMARY KEY SEMI {
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                if(db->sheet[tableID]->p_key != nullptr)db->sheet[tableID]->removePrimaryKey();
                else printf("TABLE %s doesn't have a primary key\n",$3.c_str());
            }
            db->update();
        }
    }
    | ALTER TABLE tbName ADD CONSTRAINT pkName PRIMARY KEY LB colNames RB SEMI {
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                if(db->sheet[tableID]->p_key != nullptr)printf("TABLE %s has a primary key\n",$3.c_str());
                else {
                    int* key_index = new int[$10.size()];
                    bool create_ok = true;
                    for(uint i = 0;i < $10.size();i ++){
                        bool flag = false;
                        for(uint j = 0;j < db->sheet[tableID]->col_num;j ++){
                            if ($10[i] == std::string(db->sheet[tableID]->col_ty[j].name)){
                                flag = true;
                                key_index[i] = j;
                                break;
                            }
                        }
                        if (!flag) {
                            printf("TABLE %s doesn't have a column named %s\n",$3.c_str(),$10[i].c_str());
                            create_ok = false;
                            break;
                        }
                    }
                    if(create_ok){
                        PrimaryKey temp = PrimaryKey(db->sheet[tableID], $10.size(), key_index);
                        temp.name = $6;
                        int x = db->sheet[tableID]->createPrimaryKey(&temp);
                        if (x == -2)printf("Constraint problem\n");
                    }
                }
            }
            db->update();
        }
    }
    | ALTER TABLE tbName DROP PRIMARY KEY pkName SEMI {
        //TODO: pkName?
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                if(db->sheet[tableID]->p_key != nullptr)db->sheet[tableID]->removePrimaryKey();
                else printf("TABLE %s doesn't have a primary key\n",$3.c_str());
            }
            db->update();
        }
    }
    | ALTER TABLE tbName ADD CONSTRAINT fkName FOREIGN KEY LB colNames RB REFERENCES tbName LB colNames RB SEMI {
        if(current_db_exists()){
            int tableID1;
            int tableID2;
            if(table_exists($3,tableID1,true) && table_exists($13,tableID2,true)){
                if(db->sheet[tableID2]->p_key != nullptr)printf("TABLE %s has a primary key\n",$13.c_str());
                else {
                }
            }
            db->update();
        }
    }
    | ALTER TABLE tbName DROP FOREIGN KEY fkName SEMI{
        if(current_db_exists()){
            int tableID;
            if(table_exists($3,tableID,true)){
                bool flag = false;
                for(uint i = 0;i < db->sheet[tableID]->f_key.size();i ++){
                    if(db->sheet[tableID]->f_key[i]->name == $7){
                        db->sheet[tableID]->removeForeignKey(db->sheet[tableID]->f_key[i]);
                        flag = true;
                        break;
                    }
                }
                if (!flag) printf("TABLE %s doesn't have a forrign key named %s\n",$3.c_str(),$7.c_str());
            }
            db->update();
        }
    };

fields:
    field {
        $$.push_back($1);
    }
    | fields COMMA field {
        $1.push_back($3);
        $$ = $1;
    };

field:
    colName type {
        strcpy($2.name, $1.c_str());
        $$ = $2;
    }
    | colName type NOT NL {
        strcpy($2.name, $1.c_str());
        $2.setNull(false);
        $$ = $2;
    }
    | colName type DEFAULT value {
        strcpy($2.name, $1.c_str());
        $2.def = $4;
        $$ = $2;
    }
    | colName type NOT NL DEFAULT value  {
        strcpy($2.name, $1.c_str());
        $2.setNull(false);
        $2.def = $6;
        $$ = $2;
    };

type:
    TYPE_INT {
        $$ = Type("", INT);
    }
    | TYPE_VARCHAR LB VALUE_INT RB {
        $$ = Type("", VARCHAR, $3);
    }
    | TYPE_CHAR LB VALUE_INT RB {
        $$ = Type("", CHAR, $3);
    }
    | TYPE_DATE {
        $$ = Type("", DATE);
    }
    | TYPE_FLOAT {
        $$ = Type("", DECIMAL);
    };

_values:
    value { 
        $$.push_back($1);
    }
    | LB values RB { 
        $$ = $2;
    };

values:
    value { 
        $$.push_back($1);
    }
    | values COMMA value {
        $1.push_back($3);
        $$ = $1;
    };

value:
    VALUE_INT { $$ = Any($1); }
    | VALUE_STRING { $$ = Any((char*)$1.c_str()); }
    | VALUE_DATE { 
        if (($1 / 10000 < 1500 || $1 / 10000 > 2500) || 
            ($1 % 10000 / 100 < 1 || $1 % 10000 / 100 > 12) || 
            ($1 % 100 < 1 || $1 % 100 > month[$1 % 10000 / 100 - 1]) || 
            ((($1 / 10000 % 4 != 0) ^ ($1 / 10000 % 100 != 0) ^ ($1 / 10000 % 400 != 0)) && $1 % 100 == 29)) {
            printf("Wrong date: %04d-%02d-%02d\n", $1 / 10000, $1 % 10000 / 100, $1 % 100);
            YYERROR;
        }
        $$ = Any($1); 
    }
    | VALUE_LONG_DOUBLE { $$ = Any($1); }
    | NL { $$ = Any(); };

fromClauses:
    fromClause {
        $$.push_back($1);
    }
    | fromClauses COMMA fromClause {
        $1.push_back($3);
        $$ = $1;
    };

fromClause:
    tbName {
        int idx = (db ? db->findSheet($1) : MAX_SHEET_NUM);
        if (idx < 0) {
            idx = MAX_SHEET_NUM;
            printf("Can not find TABLE %s\n", $1.c_str());
        }
        $$ = make_pair(idx, $1);
    }
    | tbName AS aliasName {
        int idx = (db ? db->findSheet($1) : MAX_SHEET_NUM);
        if (idx < 0) {
            idx = MAX_SHEET_NUM;
            printf("Can not find TABLE %s\n", $1.c_str());
        }
        $$ = make_pair(idx, $3);
    }
    | LB seleStmt RB AS aliasName {
        $$ = make_pair($2, $5);
    };

aliasName:
    IDENTIFIER { $$ = $1; };

whereClauses:
    whereClause {
        $$.push_back($1);
    }
    | whereClauses AND whereClause {
        $1.push_back($3);
        $$ = $1;
    };

whereClause:
    _cols op _values {
        $$.ty = 0;
        $$.cols = $1;
        $$.op = $2;
        $$.rvalue = $3;
    }
    | _cols op _cols {
        $$.ty = 1;
        $$.cols = $1;
        $$.op = $2;
        $$.rvalue_cols = $3;
    }
    | _cols IS NL {
        $$.ty = 2;
        $$.cols = $1;
        $$.op = OP_NL;
    }
    | _cols IS NOT NL {
        $$.ty = 3;
        $$.cols = $1;
        $$.op = OP_NNL;
    }
    | _cols op LB seleStmt RB {
        $$.ty = 4;
        $$.cols = $1;
        $$.op = $2;
        $$.rvalue_sheet = $4;
    }
    | _cols op ANY LB seleStmt RB {
        $$.ty = 5;
        $$.cols = $1;
        $$.op = $2;
        $$.rvalue_sheet = $5;
    }
    | _cols op ALL LB seleStmt RB {
        $$.ty = 6;
        $$.cols = $1;
        $$.op = $2;
        $$.rvalue_sheet = $5;
    }
    | _cols IN LB seleStmt RB {
        $$.ty = 7;
        $$.cols = $1;
        $$.op = OP_IN;
        $$.rvalue_sheet = $4;
    };

_cols:
    LB cols RB {
        $$ = $2;
    }
    | col {
        $$.push_back($1);
    };

cols:
    cols COMMA col {
        $1.push_back($3);
        $$ = $1;
    }
    | col {
        $$.push_back($1);
    };

col:
    tbName DOT colName {
        $$ = make_pair($1, $3);
    }
    | colName {
        $$ = make_pair(string(), $1);
    };

op:
    EQ { $$ = OP_EQ; }
    | NEQ { $$ = OP_NEQ; }
    | LE { $$ = OP_LE; }
    | GE { $$ = OP_GE; }
    | LS { $$ = OP_LS; }
    | GT { $$ = OP_GT; };

setClause:
    colName EQ value {
        $$.push_back(make_pair($1, $3));
    }
    | setClause COMMA colName EQ value {
        $1.push_back(make_pair($3, $5));
        $$ = $1;
    };

selector:
    STAR {}
    | cols { $$ = $1; };

_aggregations:
    LB aggregations RB {
        $$ = $2;
    }
    | aggregation {
        $$.push_back($1);
    };

aggregations:
    aggregations COMMA aggregation {
        $1.push_back($3);
        $$ = $1;
    }
    | aggregation {
        $$.push_back($1);
    };

aggregation:
    COUNT LB STAR RB AS colName {
        $$ = make_pair(make_pair(0, $6), make_pair("", ""));
    }
    | MIN LB col RB AS colName {
        $$ = make_pair(make_pair(1, $6), $3);
    }
    | MAX LB col RB AS colName {
        $$ = make_pair(make_pair(2, $6), $3);
    }
    | SUM LB col RB AS colName {
        $$ = make_pair(make_pair(3, $6), $3);
    }
    | AVG LB col RB AS colName {
        $$ = make_pair(make_pair(4, $6), $3);
    };

colNames:
    colName {
        $$.push_back($1);
    }
    | colNames COMMA colName {
        $1.push_back($3);
        $$ = $1;
    };

dbName:
    IDENTIFIER { $$ = $1; };

tbName:
    IDENTIFIER { $$ = $1; };

colName:
    IDENTIFIER { $$ = $1; };

pkName:
    IDENTIFIER { $$ = $1; };

fkName:
    IDENTIFIER { $$ = $1; };

idxName:
    IDENTIFIER { $$ = $1; };
%%

void yyerror(const char *s)
{
    cerr << s << endl;
}
