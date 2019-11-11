CXX = g++
CXXFLAGS = --std=c++11 -Wall -g

SRCS = $(wildcard *.cpp)
OBJECTS = $(SRCS:.cpp=.o)
HEADERS = $(wildcard *.h)

.PHONY: all clean

all: useless

useless: $(OBJECTS)
	$(CXX) $^ $(CXXFLAGS) -o $@

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o useless
