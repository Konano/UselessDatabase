%{
#include "parser.h"

extern "C"
{
    void yyerror(const char *s);
    extern int yylex(void);
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
%token INT
%token VARCHAR
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
%token DATE
%token FLOAT
%token FOREIGN
%token ON
%token TO
%token IDENTIFIER
%token VALUE_INT
%token VALUE_STRING

%%

program:
    | program stmt;

stmt:
    sysStmt ";"
    | dbStmt ";"
    | tbStmt ";"
    | idxStmt ";"
    | alterStmt ";";

sysStmt:
    SHOW DATABASES;

dbStmt:
    CREATE DATABASE dbName
    | DROP DATABASE dbName
    | USE dbName
    | SHOW TABLES;

tbStmt:
    CREATE TABLE tbName "(" fieldList ")"
    | DROP TABLE tbName
    | DESC tbName
    | INSERT INTO tbName VALUES valueLists
    | DELETE FROM tbName WHERE whereClause
    | UPDATE tbName SET setClause WHERE whereClause
    | SELECT selector FROM tableList WHERE whereClause;

idxStmt:
    CREATE INDEX idxName ON tbName " (" columnList ") "
    | DROP INDEX idxName
    | ALTER TABLE tbName ADD INDEX idxName "(" columnList ")"
    | ALTER TABLE tbName DROP INDEX idxName;

alterStmt:
    ALTER TABLE tbName ADD field
    | ALTER TABLE tbName DROP colName
    | ALTER TABLE tbName CHANGE colName field
    | ALTER TABLE tbName RENAME TO tbName
    | ALTER TABLE tbName ADD PRIMARY KEY " (" columnList ") "
    | ALTER TABLE tbName DROP PRIMARY KEY
    | ALTER TABLE tbName ADD CONSTRAINT pkName PRIMARY KEY " (" columnList ") "
    | ALTER TABLE tbName DROP PRIMARY KEY pkName
    | ALTER TABLE tbName ADD CONSTRAINT fkName FOREIGN KEY " (" columnList ") " REFERENCES tbName " (" columnList ") "
    | ALTER TABLE tbName DROP FOREIGN KEY fkName;

fieldList:
    field
    | fieldList "," field;

field:
    colName type
    | colName type NOT NL
    | colName type DEFAULT value
    | colName type NOT NL DEFAULT value
    | PRIMARY KEY "(" columnList ")"
    | FOREIGN KEY "(" colName ")" REFERENCES tbName "(" colName ")";

type:
    INT "(" VALUE_INT ")"
    | VARCHAR "(" VALUE_INT ")"
    | DATE
    | FLOAT;

valueLists:
    "(" valueList ")"
    | valueLists ", " " (" valueList ") ";

valueList:
    value
    | valueList ", " value;

value:
    VALUE_INT
    | VALUE_STRING
    | NL;

whereClause:
    col op expr
    | col IS [NOT] NL
    | whereClause AND whereClause;

col:
    tbName "." colName
    | colName;

op:
    "=" | "<>" | "<=" | ">=" | "<" | ">";

expr:
    value
    | col;

setClause:
    colName "=" value
    | setClause ", " colName "=" value;

selector:
    "*"
    | selector_;

selector_:
    selector_ "," col 
    | col;

tableList:
    tbName
    | tableList "," tbName;

columnList:
    colName
    | columnList "," colName;

dbName:
    IDENTIFIER;

tbName:
    IDENTIFIER;

colName:
    IDENTIFIER;

pkName:
    IDENTIFIER;

fkName:
    IDENTIFIER;

idxName:
    IDENTIFIER;
%%

void yyerror(const char *s)
{
    cerr << s << endl;
}
