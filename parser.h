#ifndef __PARSER
#define __PARSER

#include "Any.h"
#include "Type.h"
#include "Database.h"
#include "Sheet.h"

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct YYType_Where {
	uint ty;
	vector<Pss> cols;
	enumOp op;
	vector<Any> rvalue;
	vector<Pss> rvalue_cols;
	int rvalue_sheet;
};

struct YYType {
	string S;
	int I;
	uint32_t U;
	long double D;
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
	vector<Psa> V_P_SA;
};

#define YYSTYPE YYType

#endif
