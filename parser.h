#ifndef __PARSER
#define __PARSER

#include "Any.h"
#include "Type.h"

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

#include "Database.h"
#include "Sheet.h"

using namespace std;

struct YYType_Where {
	uint ty;
	vector<Pss> cols;
	enumOp op;
	Any rvalue;
	vector<Pss> rvalue_cols;
	int rvalue_sheet;
};

struct YYType {
	string S;
	int I;
	// char C;
	Type TY;
	vector<Type> V_TY;
	Any V;
	vector<Any> V_V;
	vector<string> V_S;
	Pss P_SS;
	vector<Pss> V_P_SS;
	Pis P_IS;
	vector<Pis> V_P_IS;
	enumOp eOP;
	YYType_Where W;
	vector<YYType_Where> V_W;

	// SelectStmt SEL;
	// WhereStmt WH;
};

#define YYSTYPE YYType

#endif
