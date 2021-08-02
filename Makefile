CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb
CXXFLAGS=-Wall -Wextra -std=c++17 -pedantic -ggdb

.PHONY: all
all: example-c example-cxx

example-c: example.c flag.h
	$(CC) $(CFLAGS) -o example-c example.c

example-cxx: example.c flag.h
	$(CXX) $(CXXFLAGS) -x c++ -o example-cxx example.c
