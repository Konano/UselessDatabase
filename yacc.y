%{
#include "parser.h"
#include "Sheet.h"
#include "Database.h"
#include "Type.h"

extern int cleanFiles(const char *dir);

extern "C"
{
    void yyerror(const char *s);
    extern int yylex(void);
};

Database* db;

vector<Piu> dealCol(vector<Pss> cols, vector<Pis> from) {
	vector<Piu> v; 
	for (auto it: cols) {
		for (auto _it: from) if (it.first == _it.second) {
			int col;
			if (_it.first < 0) {
				col = db->sel_sheet[-1-_it.first]->findCol(it.second);
			} else {
				col = db->sheet[_it.first]->findCol(it.second);
			}
			// TODO if (col < 0)
			v.push_back(make_pair(_it.first, (uint)col));
			break;
		}
	}
	return v;
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
%token <S> IDENTIFIER
%token <I> VALUE_INT
%token <S> VALUE_STRING
%type <S> dbName tbName colName pkName fkName idxName aliasName
%type <TY> type
%type <TY> field
%type <V_TY> fieldList
%type <V> value
%type <V_V> valueList
%type <P_SS> col
%type <V_P_SS> selector cols _cols
%type <I> seleStmt
%type <P_IS> fromClause
%type <V_P_IS> fromClauses
%type <W> whereClause
%type <V_W> whereClauses
%type <eOP> op

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
    SHOW DATABASES;

dbStmt:
    CREATE DATABASE dbName SEMI {
        // TODO Error handling
        new Database($3.c_str(), true);
    }
    | DROP DATABASE dbName SEMI {
        // TODO Error handling
        cleanFiles($3.c_str());
    }
    | USE dbName SEMI {
        // TODO Error handling
        if (db) delete db;
        db = new Database($2.c_str(), false);
    }
    | SHOW TABLES SEMI {
        // TODO Error handling
        db->showSheets();
    };

tbStmt:
    CREATE TABLE tbName LB fieldList RB SEMI {
        // TODO Error handling
        Type* ty = new Type[$5.size()];
        for (uint i = 0; i < $5.size(); i++) ty[i] = $5[i];
        db->createSheet($3.c_str(), $5.size(), ty);
    }
    | DROP TABLE tbName SEMI 
    | DESC tbName SEMI {
        // TODO Error handling
        int idx = db->findSheet($2);
        db->sheet[idx]->printCol();
    }
    | INSERT INTO tbName VALUES valueLists SEMI 
    | DELETE FROM tbName WHERE whereClauses SEMI 
    | UPDATE tbName SET setClause WHERE whereClauses SEMI
    | seleStmt SEMI {
        db->querySel(-1 - $1);
        db->sel_sheet[-1 - $1]->print();
        db->sel_num = 0;
    };

seleStmt:
    SELECT selector FROM fromClauses {
        uint idx = db->sel_num++;
        db->sel_sheet[idx]->sel = true;
        db->sel[idx].select = dealCol($2, $4);
        db->sel[idx].form = $4;
        for (auto it: $4) if (it.first < 0) {
            db->sel[idx].recursion.push_back(it.first);
        }
        $$ = -1 - idx;
    }
    | SELECT selector FROM fromClauses WHERE whereClauses {
        uint idx = db->sel_num++;
        db->sel[idx].select = dealCol($2, $4);
        db->sel[idx].form = $4;
        for (auto it: $4) if (it.first < 0) {
            db->sel[idx].recursion.push_back(it.first);
        }

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
                break;
            case 1:
                where.rvalue_ty = 2;
                break;
            case 2:
            case 3:
                break;
            case 4:
            case 5:
            case 6:
            case 7:
                where.rvalue_ty = 3;
                break;
            }
            if (where.rvalue_sheet < 0) {
                db->sel[idx].recursion.push_back(where.rvalue_sheet);
            }
            db->sel[idx].where.push_back(where);
        }

        $$ = -1 - idx;
    };

idxStmt:
    CREATE INDEX idxName ON tbName LB columnList RB SEMI 
    | DROP INDEX idxName SEMI 
    | ALTER TABLE tbName ADD INDEX idxName LB columnList RB SEMI 
    | ALTER TABLE tbName DROP INDEX idxName SEMI;

alterStmt:
    ALTER TABLE tbName ADD field SEMI 
    | ALTER TABLE tbName DROP colName SEMI 
    | ALTER TABLE tbName CHANGE colName field SEMI 
    | ALTER TABLE tbName RENAME TO tbName SEMI 
    | ALTER TABLE tbName ADD PRIMARY KEY LB columnList RB SEMI 
    | ALTER TABLE tbName DROP PRIMARY KEY SEMI 
    | ALTER TABLE tbName ADD CONSTRAINT pkName PRIMARY KEY LB columnList RB SEMI 
    | ALTER TABLE tbName DROP PRIMARY KEY pkName SEMI 
    | ALTER TABLE tbName ADD CONSTRAINT fkName FOREIGN KEY LB columnList RB REFERENCES tbName LB columnList RB SEMI 
    | ALTER TABLE tbName DROP FOREIGN KEY fkName SEMI;

fieldList:
    field {
        $$.push_back($1);
    }
    | fieldList COMMA field {
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
    | colName type DEFAULT value
    | colName type NOT NL DEFAULT value;

type:
    TYPE_INT {
        $$ = Type("", INT);
    }
    | TYPE_VARCHAR LB VALUE_INT RB
    | TYPE_CHAR LB VALUE_INT RB {
        $$ = Type("", CHAR, $3);
    }
    | TYPE_DATE
    | TYPE_FLOAT
    ;

valueLists:
    LB valueList RB
    | valueLists COMMA LB valueList RB;

valueList:
    value { 
        $$.push_back($1);
    }
    | valueList COMMA value {
        $1.push_back($3);
        $$ = $1;
    };

value:
    VALUE_INT {
        $$ = Any($1);
    }
    | VALUE_STRING {
        $$ = Any($1.c_str());
    }
    | NL {
        $$ = Any();
    };

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
        int idx = db->findSheet($1);
        // TODO if (idx < 0) ...
        $$ = make_pair(idx, $1);
    }
    | tbName AS aliasName {
        int idx = db->findSheet($1);
        // TODO if (idx < 0) ...
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
    _cols op value {
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
    colName EQ value
    | setClause COMMA colName EQ value;

selector:
    STAR {}
    | cols;

cols:
    cols COMMA col {
        $1.push_back($3);
        $$ = $1;
    }
    | col {
        $$.push_back($1);
    };

columnList:
    colName
    | columnList COMMA colName;

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
