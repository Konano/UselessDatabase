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
%token LB
%token RB
%token SEMI
%token COMMA
%token STAR
%token EQ
%token NEQ
%token LE
%token GE
%token LS
%token GT
%token DOT
%token <S> IDENTIFIER
%token <I> VALUE_INT
%token <S> VALUE_STRING
%type <S> dbName
%type <S> tbName
%type <S> colName
%type <S> pkName
%type <S> fkName
%type <S> idxName
%type <TY> type
%type <TY> field
%type <V_TY> fieldList
%type <V> value
%type <V_V> valueList
%type <V_S> tableList

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
    }
    ;

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
    | DELETE FROM tbName WHERE whereClause SEMI 
    | UPDATE tbName SET setClause WHERE whereClause SEMI
    | SELECT selector FROM tableList SEMI {
        int idx = db->findSheet($4[0]); // TODO if I can't find?
        db->sheet[idx]->print();
    }
    | SELECT selector FROM tableList WHERE whereClause SEMI;

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

field: // TODO UNIQUE?
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
    | colName type NOT NL DEFAULT value
    | PRIMARY KEY LB columnList RB
    | FOREIGN KEY LB colName RB REFERENCES tbName LB colName RB;

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

whereClause:
    col op expr
    | col IS [NOT] NL
    | whereClause AND whereClause;

col:
    tbName DOT colName
    | colName;

op:
    EQ | NEQ | LE | GE | LS | GT;

expr:
    value
    | col;

setClause:
    colName EQ value
    | setClause COMMA colName EQ value;

selector:
    STAR
    | selector_;

selector_:
    selector_ COMMA col 
    | col;

tableList:
    tbName {
        $$.push_back($1);
    }
    | tableList COMMA tbName {
        $1.push_back($3);
        $$ = $1;
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
