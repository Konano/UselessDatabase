CXX = g++
LEX = flex
YACC = bison
CXXFLAGS = --std=c++14 -Wall -g
CXXFLAGS_UNUSED_CLOSE = -Wno-unused-function

SRCS = $(wildcard *.cpp)
OBJECTS = $(SRCS:.cpp=.o)
HEADERS = $(wildcard *.h)
HEADERS := $(filter-out yacc.tab.h parser.h, $(HEADERS))

.PHONY: all clean

all: useless

useless: $(OBJECTS) lex.yy.o  yacc.tab.o
	$(CXX) $^ $(CXXFLAGS) -o $@

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

lex.yy.o: lex.yy.c yacc.tab.h parser.h
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_UNUSED_CLOSE) -c $<

yacc.tab.o: yacc.tab.c parser.h
	$(CXX) $(CXXFLAGS) -c $<

yacc.tab.c yacc.tab.h: yacc.y
	$(YACC) -d $<

lex.yy.c: lex.l
	$(LEX) $<

clean:
	rm -f *.o useless yacc.tab.* lex.yy.*
