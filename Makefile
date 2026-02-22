CFLAGS=-Wall -Wextra -std=c99 -pedantic -ggdb
CXXFLAGS=-Wall -Wextra -Wno-missing-field-initializers -std=c++17 -pedantic -ggdb

.PHONY: all clean
all: example-c example-cxx

example-c: example.c flag.h
	$(CC) $(CFLAGS) -o example-c example.c

example-cxx: example.c flag.h
	$(CXX) $(CXXFLAGS) -x c++ -o example-cxx example.c

clean:
	rm -rf example-c example-cxx example-c.dSYM example-cxx.dSYM
