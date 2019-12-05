#include "Print.h"

#include <iostream>

std::string lenLimit(std::string str, uint len) {
    if (str.length() > len) 
        return str.substr(0, len-2) + ".."; 
    else 
        return str + std::string(len - str.length(), ' ');
}

std::string toString(Any val) {
    if (val.anyCast<int>() != NULL) {
        return std::to_string(*val.anyCast<int>());
    }
    if (val.anyCast<char*>() != NULL) {
        return std::string(*val.anyCast<char*>());
    }
    return "NULL";
}

std::vector<std::pair<std::string,int> >Print::t;
int Print::col_num;

void Print::title(std::vector<std::pair<std::string,int> > v) {
    t = v;
    col_num = v.size();

    std::string str = "+";
    for (int i = 0; i < col_num; i++) str += std::string(2 + t[i].second, '-') + '+';
    std::cout << str << std::endl;

    str = "|";
    for (int i = 0; i < col_num; i++) str += ' ' + lenLimit(t[i].first, t[i].second) + " |";
    std::cout << str << std::endl;

    str = "+";
    for (int i = 0; i < col_num; i++) str += std::string(2 + t[i].second, '-') + '+';
    std::cout << str << std::endl;
}

void Print::row(std::vector<Any> v) {
    std::string str = "|";
    for (int i = 0; i < col_num; i++) str += ' ' + lenLimit(toString(v[i]), t[i].second) + " |";
    std::cout << str << std::endl;
}

void Print::end() {
    std::string str = "+";
    for (int i = 0; i < col_num; i++) str += std::string(2 + t[i].second, '-') + '+';
    std::cout << str << std::endl;
    t.clear();
    col_num = 0;
}