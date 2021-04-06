NAME = FCompiler

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

.PHONY: build link clean setup install
.SUFFIXES: .o .c
build: bin/compiler.o
	$(CC) bin/compiler.o -o bin/output.elf $(CFLAGS)

bin/compiler.o: src/compiler.c
	$(CC) -c src/compiler.c -o bin/compiler.o $(CFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $^ -o $@
link:
	$(AR) rcs bin/libc.a $(OBJS)
	cp $(ARCHDIR)/crt*.o bin/
install:
	mkdir -p ../sysroot/usr/lib/
	cp bin/* ../sysroot/usr/lib/
clean:
	rm -rf $(BUILDDIR)
	rm -f $(OBJS)
setup:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(SRCDIR)
	@mkdir -p $(ARCHDIR)
	@mkdir -p $(INCLUDEDIR)
