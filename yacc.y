%{
#include "parser.h"
#include "Table.h"
#include "Database.h"
#include "Type.h"

extern int cleanDatabase(const char *dbname);
extern char* dirPath(const char* dir, const char* path);
extern int checkFile(const char *filename);
extern void showDatabases();
extern string Type2Str(enumType ty);
extern string Aggr2Str(enumAggr ty);
extern string Any2Str(Any val);

extern "C"
{
    void yyerror(const char *s);
    extern int yylex(void);
};

extern Database* db;
bool error;

#define filterTable(it) ((it) < 0 ? db->sel_table[-1-it] : db->table[it])
#define Piu2Col(it) (filterTable((it).first)->col_ty[(it).second])

uint month[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

bool current_db_exists() {
    if (db == nullptr) printf("Select a database first\n");
    return db != nullptr;
}

bool table_exists(std::string name, int& tableID, bool expect) {
    tableID = db->findTable(name);
    if (tableID == -1 && expect) printf("TABLE %s doesn't exist\n", name.c_str());
    if (tableID != -1 && !expect) printf("TABLE %s exists\n", name.c_str());
    return tableID != -1;
}

Piu dealCol(Pss col, vector<Pis> &from) {
    if (col.first == string() && from.size() > 1) {
        printf("COLUMN %s need to assign TABLE\n", col.second.c_str());
        error = true;
        return Piu(0, 0);
    }

    for (auto _it: from) if ((col.first == _it.second) || (col.first == string() && from.size() == 1)) {
        int colIndex = filterTable(_it.first)->findCol(col.second);
        if (colIndex < 0) {
            printf("COLUMN %s doesn't exist in TABLE %s\n", col.second.c_str(), _it.second.c_str());
            error = true;
            return Piu(0, 0);
        }
        return Piu(_it.first, (uint)colIndex);
    }
    
    printf("TABLE %s doesn't exist\n", col.first.c_str());
    error = true;
    return Piu(0, 0);
}

inline vector<Piu> dealCols(vector<Pss> &cols, vector<Pis> &from) {
	vector<Piu> v; 
	for (auto col: cols) v.push_back(dealCol(col, from));
	return v;
}

void dealTy(uint idx) {
	if (db->sel[idx].select.size() == 0) {
        if (db->sel[idx].aggr.size() == 0) {
            for (auto it: db->sel[idx].from) {
                Table* st = filterTable(it.first);
                for (uint i = 0; i < st->col_num; i++) {
                    db->sel_table[idx]->col_ty[db->sel_table[idx]->col_num++] = st->col_ty[i];
                }
            }
        } else {
           for (auto it: db->sel[idx].aggr) switch (it.ty) {
                case AG_COUNT: { 
                   db->sel_table[idx]->col_ty[db->sel_table[idx]->col_num++] = Type(it.as.c_str(), INT);
                   break;
                }
                case AG_MAX:
                case AG_MIN: {
                    db->sel_table[idx]->col_ty[db->sel_table[idx]->col_num++] = Type(it.as.c_str(), filterTable(it.col.first)->col_ty[it.col.second].ty);
                    break;
                }
                case AG_SUM: 
                case AG_AVG: {
                    if (filterTable(it.col.first)->col_ty[it.col.second].ty != INT && filterTable(it.col.first)->col_ty[it.col.second].ty != DECIMAL) {
                        printf("Use %s for %s is illegal\n", Aggr2Str(it.ty).c_str(), Type2Str(filterTable(it.col.first)->col_ty[it.col.second].ty).c_str());
                        error = true;
                        return;
                    }
                    db->sel_table[idx]->col_ty[db->sel_table[idx]->col_num++] = Type(it.as.c_str(), filterTable(it.col.first)->col_ty[it.col.second].ty);
                    break;
                }
            }
        }
    } else {
        for (auto it: db->sel[idx].select) {
            db->sel_table[idx]->col_ty[db->sel_table[idx]->col_num++] = filterTable(it.first)->col_ty[it.second];
        }
    }
}

inline bool tycheck_any(Type ty, Any v) {
    switch (ty.ty) {
        case INT: 
            return v.anyCast<int>() != nullptr;
        case CHAR: 
        case VARCHAR: 
            return (v.anyCast<char*>() != nullptr) && (strlen((const char*)v.anyCast<char*>()) <= ty.char_len);
        case DATE: 
            return v.anyCast<uint32_t>() != nullptr;
        case DECIMAL: 
            return v.anyCast<long double>() != nullptr;
        default:
            return false;
    }
}

inline bool tycheck_ty(Type ty, Type _ty) {
    if (ty.ty == _ty.ty) return true;
    if (ty.ty == CHAR && _ty.ty == VARCHAR) return true;
    if (ty.ty == VARCHAR && _ty.ty == CHAR) return true;
    return false;
}

inline const char *op2str(enumOp op) {
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

WhereStmt dealWhere(YYType_Where &it, vector<Pis> &from) {
    WhereStmt where;
    where.cols = dealCols(it.cols, from);
    where.op = it.op;
    where.rvalue = it.rvalue;
    where.rvalue_cols = dealCols(it.rvalue_cols, from);
    where.rvalue_table = it.rvalue_table;
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
                    filterTable(where.cols[i].first)->name, 
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
                    filterTable(where.cols[i].first)->name, 
                    Piu2Col(where.cols[i]).name, 
                    Type2Str(Piu2Col(where.cols[i]).ty).c_str(),
                    op2str(where.op), 
                    filterTable(where.rvalue_cols[i].first)->name, 
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
        if (it.ty == 4 && filterTable(where.rvalue_table)->sel != 2) {
            printf("use %s for TABLE is illegal\n", op2str(where.op));
            break;
        }
        if (it.ty > 4 && filterTable(where.rvalue_table)->sel == 2) {
            printf("use %s for VALUELIST is illegal\n", op2str(where.op));
            break;
        }
        if (where.cols.size() != filterTable(where.rvalue_table)->col_num) {
            printf("length is not equal\n");
            break;
        }
        for (uint i = 0, size = where.cols.size(); i < size; i++) {
            if (tycheck_ty(Piu2Col(where.cols[i]), filterTable(where.rvalue_table)->col_ty[i]) == false) {
                printf("Type error: %s.%s(%s) %s %s.%s(%s)\n", 
                    filterTable(where.cols[i].first)->name, 
                    Piu2Col(where.cols[i]).name, 
                    Type2Str(Piu2Col(where.cols[i]).ty).c_str(),
                    op2str(where.op), 
                    filterTable(where.rvalue_table)->name, 
                    filterTable(where.rvalue_table)->col_ty[i].name,
                    Type2Str(filterTable(where.rvalue_table)->col_ty[i].ty).c_str());
                error = true;
            }
        }
        break;
    }
    return where;
}

bool check_datatype(Type tar, Any val) {
    if (val.anyCast<char*>() != nullptr) {
        return tar.ty == enumType::CHAR || tar.ty == enumType::VARCHAR;
    } else if (val.anyCast<int>() != nullptr) {
        return tar.ty == enumType::INT;
    } else if (val.anyCast<long double>() != nullptr) {
        return tar.ty == enumType::DECIMAL;
    } else if (val.anyCast<uint32_t>() != nullptr) {
        return tar.ty == enumType::DATE;
    }
    else return true;
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
%type <G> aggrClause
%type <V_G> aggrClauses _aggrClauses

%%

program:
    | EXIT {
        if (db != nullptr)delete db;
        return 0;
    }
    | stmt program;

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
            printf("DATABASE %s exists\n", $3.c_str());
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
            printf("DATABASE %s doesn't exist\n", $3.c_str());
        }
    }
    | USE dbName SEMI {
        if (checkFile(dirPath($2.c_str(), ".storage")) == 1) {
            if (db) delete db;
            db = new Database($2.c_str(), false);
        } else {
            printf("DATABASE %s doesn't exist\n", $2.c_str());
        }
    }
    | SHOW TABLES SEMI {
        if (current_db_exists()) {
            db->showTables();
        }
    };

tbStmt:
    CREATE TABLE tbName LB fields RB SEMI {
        if (current_db_exists()) {
            int tableID;
            if (!table_exists($3, tableID, false)) {
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
                    db->createTable($3.c_str(), $5.size(), ty);
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
            if (db->deleteTable($3.c_str())) printf("TABLE %s doesn't exist\n", $3.c_str());
        }
    }
    | DESC tbName SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($2, tableID, true)) {
                db->table[tableID]->printCol();
            }
        }
    }
    | INSERT INTO tbName VALUES LB values RB SEMI{
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                if (db->table[tableID]->col_num == $6.size()) {
                    bool flag = true;
                    for (uint i = 0; i < $6.size(); i++) {
                        if (!check_datatype(db->table[tableID]->col_ty[i], $6[i])) {
                            flag = false;
                            break;
                        }
                    }
                    if (flag) {
                        Any* temp = new Any[db->table[tableID]->col_num];
                        for (uint i = 0; i < $6.size(); i++)temp[i] = $6[i];
                        int x = db->table[tableID]->insertRecord(temp);
                        if (x != 0)printf("Insert fail %d\n",x);
                    }
                    else printf("Data type mismatch\n");
                }
                else printf("Column number doesn't match\n");
            }
            db->update();
        }
    }
    | INSERT INTO tbName LB colNames RB VALUES LB values RB SEMI{
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                if ($5.size() == $9.size()) {
                    vector <uint> key;
                    bool colNamecheck = true;
                    for (uint i = 0; i < $5.size(); i++) {
                        bool flag = false;
                        for (uint j = 0; j < db->table[tableID]->col_num; j++) {
                            if ($5[i] == std::string(db->table[tableID]->col_ty[j].name)) {
                                flag = true;
                                key.push_back(j);
                                break;
                            }
                        }
                        if (!flag) {
                            colNamecheck = false;
                            printf("TABLE %s doesn't have a column named %s\n", $3.c_str(), $5[i].c_str());
                            break;
                        }
                    }
                    if (colNamecheck) {
                        bool flag = true;
                        for (uint i = 0; i < key.size(); i++) {
                            if (!check_datatype(db->table[tableID]->col_ty[key[i]], $9[i])) {
                                flag = false;
                                break;
                            }
                        }
                        if (flag) {
                            Any* temp = new Any[db->table[tableID]->col_num];
                            for (uint i = 0; i < $9.size(); i++)temp[key[i]] = $9[i];
                            int x = db->table[tableID]->insertRecord(temp);
                            if (x != 0)printf("Insert fail %d\n",x);
                        }
                        else printf("Data type mismatch\n");
                    }
                }
                else printf("Column number doesn't match\n");
            }
            db->update();
        }
    }
    | DELETE FROM tbName WHERE whereClauses SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                error = false;
                vector<Pis> from; 
                from.push_back(Pis(tableID, $3));
                vector<WhereStmt> where;
                for (auto it: $5) where.push_back(dealWhere(it, from));
                if (error) YYERROR;
                for (auto it: where) if (it.rvalue_table < 0) db->buildSel(-1 - it.rvalue_table);
                db->table[tableID]->removeRecords(where);
            }
        }
    }
    | UPDATE tbName SET setClause WHERE whereClauses SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($2, tableID, true)) {
                error = false;
                vector<Pis> from; 
                from.push_back(Pis(tableID, $2));
                vector<Pia> set;
                for (auto it: $4) {
                    int colIndex = db->table[tableID]->findCol(it.first);
                    if (colIndex < 0) {
                        printf ("COLUMN %s doesn't exist in TABLE %s\n", $2.c_str(), it.first.c_str());
                        error = true;
                    } else {
                        set.push_back(Pia(colIndex, it.second));
                    }
                }
                vector<WhereStmt> where;
                for (auto it: $6) where.push_back(dealWhere(it, from));
                if (error) YYERROR;
                for (auto it: where) if (it.rvalue_table < 0) db->buildSel(-1 - it.rvalue_table);
                db->table[tableID]->updateRecords(set, where);
            }
        }
    }
    | seleStmt SEMI {
        if (current_db_exists()) {
            db->buildSel(-1 - $1, true);
            // db->sel_table[-1 - $1]->print();
            while (db->sel_num) {
                delete db->sel_table[--(db->sel_num)];
                db->sel[db->sel_num].build = false;
            }
        }
    };

seleStmt:
    SELECT selector FROM fromClauses {
        if (current_db_exists()) {
            for (auto it: $4) if (it.first == MAX_TABLE_NUM) YYERROR;
            error = false;
            uint idx = db->sel_num++;
            db->sel_table[idx] = new Table(1);
            db->sel[idx].select = dealCols($2, $4);
            db->sel[idx].aggr.clear();
            db->sel[idx].from = $4;
            db->sel[idx].recursion.clear();
            for (auto it: $4) if (it.first < 0) {
                db->sel[idx].recursion.push_back(it.first);
            }
            db->sel[idx].where.clear();
            dealTy(idx);
            $$ = -1 - idx;
            
            if (error) {
                delete db->sel_table[--db->sel_num];
                YYERROR;
            }
        } else YYERROR;
    }
    | SELECT _aggrClauses FROM fromClauses {
        if (current_db_exists()) {
            for (auto it: $4) if (it.first == MAX_TABLE_NUM) YYERROR;
            error = false;
            uint idx = db->sel_num++;
            db->sel_table[idx] = new Table(2);
            db->sel[idx].select.clear();
            db->sel[idx].aggr.clear();
            for (auto it: $2) if (it.ty == AG_COUNT) {
                db->sel[idx].aggr.push_back(AggrStmt{it.ty, Piu(0, 0), it.as});
            } else {
                db->sel[idx].aggr.push_back(AggrStmt{it.ty, dealCol(it.col, $4), it.as});
            }
            db->sel[idx].from = $4;
            db->sel[idx].recursion.clear();
            for (auto it: $4) if (it.first < 0) db->sel[idx].recursion.push_back(it.first);
            db->sel[idx].where.clear();
            dealTy(idx);
            $$ = -1 - idx;
            
            if (error) {
                delete db->sel_table[--db->sel_num];
                YYERROR;
            }
        } else YYERROR;
    }
    | SELECT selector FROM fromClauses WHERE whereClauses {
        if (current_db_exists()) {
            for (auto it: $4) if (it.first == MAX_TABLE_NUM) YYERROR;
            error = false;
            uint idx = db->sel_num++;
            db->sel_table[idx] = new Table(1);
            db->sel[idx].select = dealCols($2, $4);
            db->sel[idx].aggr.clear();
            db->sel[idx].from = $4;
            db->sel[idx].recursion.clear();
            for (auto it: $4) if (it.first < 0) {
                db->sel[idx].recursion.push_back(it.first);
            }
            db->sel[idx].where.clear();
            for (auto it: $6) db->sel[idx].where.push_back(dealWhere(it, $4));
            for (auto it: db->sel[idx].where) if (it.rvalue_table < 0) {
                db->sel[idx].recursion.push_back(it.rvalue_table);
            }
            dealTy(idx);
            $$ = -1 - idx;
            
            if (error) {
                delete db->sel_table[--db->sel_num];
                YYERROR;
            }
        } else YYERROR;
    }
    | SELECT _aggrClauses FROM fromClauses WHERE whereClauses {
        if (current_db_exists()) {
            for (auto it: $4) if (it.first == MAX_TABLE_NUM) YYERROR;
            error = false;
            uint idx = db->sel_num++;
            db->sel_table[idx] = new Table(2);
            db->sel[idx].select.clear();
            db->sel[idx].aggr.clear();
            for (auto it: $2) if (it.ty == AG_COUNT) {
                db->sel[idx].aggr.push_back(AggrStmt{it.ty, Piu(0, 0), it.as});
            } else {
                db->sel[idx].aggr.push_back(AggrStmt{it.ty, dealCol(it.col, $4), it.as});
            }
            db->sel[idx].from = $4;
            db->sel[idx].recursion.clear();
            for (auto it: $4) if (it.first < 0) {
                db->sel[idx].recursion.push_back(it.first);
            }
            db->sel[idx].where.clear();
            for (auto it: $6) db->sel[idx].where.push_back(dealWhere(it, $4));
            for (auto it: db->sel[idx].where) if (it.rvalue_table < 0) {
                db->sel[idx].recursion.push_back(it.rvalue_table);
            }
            dealTy(idx);
            $$ = -1 - idx;
            
            if (error) {
                delete db->sel_table[--db->sel_num];
                YYERROR;
            }
        } else YYERROR;
    };

idxStmt:
    CREATE INDEX idxName ON tbName LB colNames RB SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($5, tableID, true)) {
                if (db->table[tableID]->findIndex($3) != -1) printf("INDEX %s exists\n", $3.c_str());
                else{
                    vector<uint> key_index;
                    bool create_ok = true;
                    for (uint i = 0; i < $7.size(); i++) {
                        bool flag = false;
                        for (uint j = 0; j < db->table[tableID]->col_num; j++) {
                            if ($7[i] == std::string(db->table[tableID]->col_ty[j].name)) {
                                flag = true;
                                key_index.push_back(j);
                                break;
                            }
                        }
                        if (!flag) {
                            printf("TABLE %s doesn't have a column named %s\n", $5.c_str(), $7[i].c_str());
                            create_ok = false;
                            break;
                        }
                    }
                    if (create_ok) {
                        db->table[tableID]->createIndex(key_index, $3);
                    }
                }
            }
            db->update();
        }
    }
    | DROP INDEX idxName SEMI {
        if (current_db_exists()) {
            bool flag = false;
            for (uint i = 0; i < db->table_num; i++) {
                int indexID = db->table[i]->findIndex($3);
                if (indexID != -1) {
                    db->table[i]->removeIndex(indexID);
                    flag = true;
                    break;
                }
            }
            if (!flag) printf("INDEX %s doesn't exist\n", $3.c_str());
            db->update();
        }
    }
    | ALTER TABLE tbName ADD INDEX idxName LB colNames RB SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                if (db->table[tableID]->findIndex($6) != -1) printf("INDEX %s exists\n", $6.c_str());
                else{
                    vector<uint> key_index;
                    bool create_ok = true;
                    for (uint i = 0; i < $8.size(); i++) {
                        bool flag = false;
                        for (uint j = 0; j < db->table[tableID]->col_num; j++) {
                            if ($8[i] == std::string(db->table[tableID]->col_ty[j].name)) {
                                flag = true;
                                key_index.push_back(j);
                                break;
                            }
                        }
                        if (!flag) {
                            printf("TABLE %s doesn't have a column named %s\n", $3.c_str(), $8[i].c_str());
                            create_ok = false;
                            break;
                        }
                    }
                    if (create_ok) {
                        db->table[tableID]->createIndex(key_index, $6);
                    }
                }
            }
            db->update();
        }
    }
    | ALTER TABLE tbName DROP INDEX idxName SEMI{
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                int indexID = db->table[tableID]->findIndex($6);
                if (indexID != -1) {
                    db->table[tableID]->removeIndex(indexID);
                }
                else printf("INDEX %s doesn't exist\n", $6.c_str());
            }
            db->update();
        }
    };

alterStmt:
    ALTER TABLE tbName ADD field SEMI{
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                if (db->table[tableID]->findCol(std::string($5.name)) != -1) {
                    printf("Column %s is used\n", $5.name);
                }
                else {
                    db->table[tableID]->createColumn($5);
                }
            }
            db->update();
        }
    }
    | ALTER TABLE tbName DROP colName SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                if (db->table[tableID]->findCol($5) != -1)db->table[tableID]->removeColumn(db->table[tableID]->findCol($5));
                else printf("Column %s doesn't exist\n", $5.c_str());
            }
            db->update();
        }
    }
    | ALTER TABLE tbName CHANGE colName field SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                if (db->table[tableID]->findCol($5) != -1) {
                    if (db->table[tableID]->findCol(std::string($6.name)) == -1 || $5 == std::string($6.name))db->table[tableID]->modifyColumn(db->table[tableID]->findCol($5), $6);
                    else printf("Column %s is used\n", $6.name);
                }
                else printf("Column %s doesn't exist\n", $5.c_str());
            }
            db->update();
        }
    }
    | ALTER TABLE tbName RENAME TO tbName SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                for (uint i = 0; i < $6.length(); i++) {
                    db->table[tableID]->name[i] = $6[i];
                }
                db->table[tableID]->name[$6.length()] = '\0';
            }
            db->update();
        }
    }
    | ALTER TABLE tbName ADD PRIMARY KEY LB colNames RB SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                if (db->table[tableID]->p_key != nullptr)printf("TABLE %s has a primary key\n", $3.c_str());
                else {
                    int* key_index = new int[$8.size()];
                    bool create_ok = true;
                    for (uint i = 0; i < $8.size(); i++) {
                        bool flag = false;
                        for (uint j = 0; j < db->table[tableID]->col_num; j++) {
                            if ($8[i] == std::string(db->table[tableID]->col_ty[j].name)) {
                                flag = true;
                                key_index[i] = j;
                                break;
                            }
                        }
                        if (!flag) {
                            printf("TABLE %s doesn't have a column named %s\n", $3.c_str(), $8[i].c_str());
                            create_ok = false;
                            break;
                        }
                    }
                    if (create_ok) {
                        PrimaryKey temp = PrimaryKey(db->table[tableID], $8.size(), key_index);
                        int x = db->table[tableID]->createPrimaryKey(&temp);
                        if (x == -2)printf("Constraint problem\n");
                    }
                }
            }
            db->update();
        }
    }
    | ALTER TABLE tbName DROP PRIMARY KEY SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                if (db->table[tableID]->p_key == nullptr) printf("TABLE %s doesn't have a primary key\n", $3.c_str());
                else if (db->table[tableID]->p_key->f.size() != 0) printf("Delete related foreign key first\n");
           	    else db->table[tableID]->removePrimaryKey();
            }
            db->update();
        }
    }
    | ALTER TABLE tbName ADD CONSTRAINT pkName PRIMARY KEY LB colNames RB SEMI {
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                if (db->table[tableID]->p_key != nullptr)printf("TABLE %s has a primary key\n", $3.c_str());
                else {
                    int* key_index = new int[$10.size()];
                    bool create_ok = true;
                    for (uint i = 0; i < $10.size(); i++) {
                        bool flag = false;
                        for (uint j = 0; j < db->table[tableID]->col_num; j++) {
                            if ($10[i] == std::string(db->table[tableID]->col_ty[j].name)) {
                                flag = true;
                                key_index[i] = j;
                                break;
                            }
                        }
                        if (!flag) {
                            printf("TABLE %s doesn't have a column named %s\n", $3.c_str(), $10[i].c_str());
                            create_ok = false;
                            break;
                        }
                    }
                    if (create_ok) {
                        PrimaryKey temp = PrimaryKey(db->table[tableID], $10.size(), key_index);
                        temp.name = $6;
                        int x = db->table[tableID]->createPrimaryKey(&temp);
                        if (x == -2) printf("Constraint problem\n");
                    }
                }
            }
            db->update();
        }
    }
    | ALTER TABLE tbName DROP PRIMARY KEY pkName SEMI {
        // TODO pkName?
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                if (db->table[tableID]->p_key == nullptr) printf("TABLE %s doesn't have a primary key\n", $3.c_str());
                else if (db->table[tableID]->p_key->f.size() != 0) printf("Delete related foreign key first\n");
                else db->table[tableID]->removePrimaryKey();
            }
            db->update();
        }
    }
    | ALTER TABLE tbName ADD CONSTRAINT fkName FOREIGN KEY LB colNames RB REFERENCES tbName LB colNames RB SEMI {
        if (current_db_exists()) {
            int tableID1;
            int tableID2;
            if (table_exists($3, tableID1, true) && table_exists($13, tableID2, true)) {
                if (db->table[tableID2]->p_key == nullptr)printf("TABLE %s doesn't have a primary key\n", $13.c_str());
                else if (db->table[tableID2]->p_key->v.size() == $15.size()) {
                    bool isok = true;
                    for (uint i = 0; i < db->table[tableID2]->p_key->v.size(); i++) {
                        if ($15[i] != std::string(db->table[tableID2]->col_ty[db->table[tableID2]->p_key->v[i]].name)) {
                            isok = false;
                            printf("Primary key column %s mismatch column name %s\n", db->table[tableID2]->col_ty[db->table[tableID2]->p_key->v[i]].name, $15[i].c_str());
                            break;
                        }
                    }
                    if (isok) {
                        bool flag = true;
                        vector <uint> key;
                        for (uint i = 0; i < $10.size(); i++) {
                            bool flag2 = false;
                            for (uint j = 0; j < db->table[tableID1]->col_num; j++) {
                                if ($10[i] == std::string(db->table[tableID1]->col_ty[j].name)) {
                                    key.push_back(j);
                                    flag2 = true;
                                    break;
                                }
                            }
                            if (!flag2) {
                                flag = false;
                                printf("Column %s doesn't exist\n", $10[i].c_str());
                                break;
                            }
                        }
                        if (flag) {
                            int* temp = new int[$10.size()];
                            for (uint i = 0; i < $10.size(); i++) {
                                temp[i] = key[i];
                            }
                            ForeignKey fkey = ForeignKey(db->table[tableID1], $10.size(), temp);
                            fkey.name = $6;
                            db->table[tableID1]->createForeignKey(&fkey, db->table[tableID2]->p_key);
                        }
                    }
                }
                else printf("Column num of target primary key mismatch\n");
            }
            db->update();
        }
    }
    | ALTER TABLE tbName DROP FOREIGN KEY fkName SEMI{
        if (current_db_exists()) {
            int tableID;
            if (table_exists($3, tableID, true)) {
                bool flag = false;
                for (uint i = 0; i < db->table[tableID]->f_key.size(); i++) {
                    if (db->table[tableID]->f_key[i]->name == $7) {
                        db->table[tableID]->removeForeignKey(db->table[tableID]->f_key[i]);
                        flag = true;
                        break;
                    }
                }
                if (!flag) printf("TABLE %s doesn't have a foreign key named %s\n", $3.c_str(), $7.c_str());
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
    | VALUE_STRING { 
        char* str = new char[$1.length() + 1];
        memcpy(str, $1.c_str(), $1.length());
        str[$1.length()] = '\0';
        $$ = Any(str);
    }
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
        int idx = (db ? db->findTable($1) : MAX_TABLE_NUM);
        if (idx < 0) {
            idx = MAX_TABLE_NUM;
            printf("Can not find TABLE %s\n", $1.c_str());
        }
        $$ = make_pair(idx, $1);
    }
    | tbName AS aliasName {
        int idx = (db ? db->findTable($1) : MAX_TABLE_NUM);
        if (idx < 0) {
            idx = MAX_TABLE_NUM;
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
        $$.rvalue_table = $4;
    }
    | _cols op ANY LB seleStmt RB {
        $$.ty = 5;
        $$.cols = $1;
        $$.op = $2;
        $$.rvalue_table = $5;
    }
    | _cols op ALL LB seleStmt RB {
        $$.ty = 6;
        $$.cols = $1;
        $$.op = $2;
        $$.rvalue_table = $5;
    }
    | _cols IN LB seleStmt RB {
        $$.ty = 7;
        $$.cols = $1;
        $$.op = OP_IN;
        $$.rvalue_table = $4;
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

_aggrClauses:
    LB aggrClauses RB {
        $$ = $2;
    }
    | aggrClause {
        $$.push_back($1);
    };

aggrClauses:
    aggrClauses COMMA aggrClause {
        $1.push_back($3);
        $$ = $1;
    }
    | aggrClause {
        $$.push_back($1);
    };

aggrClause:
    COUNT LB STAR RB AS colName {
        $$.ty = AG_COUNT;
        $$.as = $6;
    }
    | MIN LB col RB AS colName {
        $$.ty = AG_MIN;
        $$.col = $3;
        $$.as = $6;
    }
    | MAX LB col RB AS colName {
        $$.ty = AG_MAX;
        $$.col = $3;
        $$.as = $6;
    }
    | SUM LB col RB AS colName {
        $$.ty = AG_SUM;
        $$.col = $3;
        $$.as = $6;
    }
    | AVG LB col RB AS colName {
        $$.ty = AG_AVG;
        $$.col = $3;
        $$.as = $6;
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
