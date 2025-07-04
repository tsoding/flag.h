CFLAGS=-Wall -Wextra -std=c23 -pedantic -ggdb

.PHONY: all
all: example-c

example-c: example.c flag.h result.h
	$(CC) $(CFLAGS) -o example-c example.c

result.h:
	wget https://github.com/guillermocalvo/resultlib/raw/refs/tags/1.0.0/src/result.h
