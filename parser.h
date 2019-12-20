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

struct YYType_Where {
	uint ty;
	std::vector<Pss> cols;
	enumOp op;
	std::vector<Any> rvalue;
	std::vector<Pss> rvalue_cols;
	int rvalue_sheet;
};

struct YYType {
	std::string S;
	int I;
	Type TY;
	std::vector<Type> V_TY;
	Any V;
	std::vector<Any> V_V;
	std::vector<std::string> V_S;
	Pss P_SS;
	std::vector<Pss> V_P_SS;
	Pis P_IS;
	std::vector<Pis> V_P_IS;
	enumOp eOP;
	YYType_Where W;
	std::vector<YYType_Where> V_W;
	std::vector<Psa> V_P_SA;
};

#define YYSTYPE YYType

#endif
