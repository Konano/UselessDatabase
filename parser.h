#ifndef __PARSER
#define __PARSER

#include "Any.h"
#include "Type.h"

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

using namespace std;

struct YYType {
	string S;
	int I;
	// char C;
	Type TY;
	vector<Type> V_TY;
	Any V;
	vector<Any> V_V;
	vector<string> V_S;
};

#define YYSTYPE YYType

#endif
