SRCDIR = src
BUILDDIR = bin

CC = gcc

LINK_LIST=\
$(LDFLAGS) \
$(OBJS) \
$(LIBS)


INCLUDEDIR=include

CFLAGS= \
-O2 \
-g \
-F dwarf \
-Wextra \
-Wall \
-I$(INCLUDEDIR)

.PHONY: build clean
.SUFFIXES: .o .c
build: bin/compiler.o
	$(CC) bin/compiler.o -o bin/output.elf $(CFLAGS)

bin/compiler.o: src/compiler.c
	$(CC) -c src/compiler.c -o bin/compiler.o $(CFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $^ -o $@
clean:
	rm -f bin/*
