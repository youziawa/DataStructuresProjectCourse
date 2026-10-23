CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8 -Iinclude
SRCS = src/mystr.c src/seqlist.c src/linklist.c src/stack.c src/hashtable.c src/sortsearch.c src/catalog.c src/demo.c src/tests.c src/main.c

catalog.exe: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

.PHONY: test clean
test: catalog.exe
	./catalog.exe test

clean:
	rm -f catalog.exe
