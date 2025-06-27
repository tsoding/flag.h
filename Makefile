CFLAGS=-Wall -Wextra -std=c23 -pedantic -ggdb
CXXFLAGS=-Wall -Wextra -std=c++20 -pedantic -ggdb -Dtypeof=__typeof

.PHONY: all
all: example-c example-cxx

example-c: example.c flag.h result.h
	$(CC) $(CFLAGS) -o example-c example.c

example-cxx: example.c flag.h result.h
	$(CXX) $(CXXFLAGS) -x c++ -o example-cxx example.c

result.h:
	wget https://github.com/guillermocalvo/resultlib/raw/refs/tags/1.0.0/src/result.h
