#ifndef __PRINT
#define __PRINT

#include "Any.h"
#include "Anys.h"

#include <vector>
#include <string>

class Print {
public:
    static std::vector<std::pair<std::string,int> > t;
    static int col_num;
    static void title(std::vector<std::pair<std::string,int> > v);
    static void row(Anys v);
    static void end();
};

#endif