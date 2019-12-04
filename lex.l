%option caseless

%{
#include "parser.h"
#include "yacc.tab.h"

extern "C"
{
	int yywrap(void);
	int yylex(void);
}
%}

letter      ([A-Za-z])
nondigit    ([_A-Za-z])
digit       ([0-9])
integer     ({digit}+)
identifier  ({letter}({nondigit}|{digit})*)
blank_chars	([ \f\r\t\v]+)
string      (’[^’]*’)
%%

DATABASE { return DATABASE; }
DATABASES { return DATABASES; }
TABLE { return TABLE; }
TABLES { return TABLES; }
SHOW { return SHOW; }
CREATE { return CREATE; }
DROP { return DROP; }
USE { return USE; }
PRIMARY { return PRIMARY; }
KEY { return KEY; }
NOT { return NOT; }
NULL { return NL; }
INSERT { return INSERT; }
INTO { return INTO; }
VALUES { return VALUES; }
DELETE { return DELETE; }
FROM { return FROM; }
WHERE { return WHERE; }
UPDATE { return UPDATE; }
SET { return SET; }
SELECT { return SELECT; }
IS { return IS; }
INT { return INT; }
VARCHAR { return VARCHAR; }
DEFAULT { return DEFAULT; }
CONSTRAINT { return CONSTRAINT; }
CHANGE { return CHANGE; }
ALTER { return ALTER; }
ADD { return ADD; }
RENAME { return RENAME; }
DESC { return DESC; }
REFERENCES { return REFERENCES; }
INDEX { return INDEX; }
AND { return AND; }
DATE { return DATE; }
FLOAT { return FLOAT; }
FOREIGN { return FOREIGN; }
ON { return ON; }
TO { return TO; }
{identifier} { return IDENTIFIER; }
{integer} { return VALUE_INT; }
{string} { return VALUE_STRING; }
%%

int yywrap(void) { return 1; }
