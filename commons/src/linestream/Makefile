CC          ?= cc
PREFIX      ?= /home/sham/af/dirty/sys_programming_exploration/commons
INCLUDEDIR  := $(PREFIX)/include
LIBDIR      := $(PREFIX)/lib
BUILDDIR    := build

CPPFLAGS    += -I$(INCLUDEDIR)
CFLAGS      ?= -std=c99 -Wall -Wextra -Wpedantic -O2

OBJECT      := $(BUILDDIR)/linestream.o
READBUFFER_OBJECT := $(BUILDDIR)/readbuffer.o
TEST_OBJECT := $(BUILDDIR)/test_linestream.o
TEST_BINARY := $(BUILDDIR)/test_linestream
INSTALLED_OBJECT := $(LIBDIR)/linestream.o

.PHONY: all install test clean

all: install

install: $(INSTALLED_OBJECT)

$(INSTALLED_OBJECT): $(OBJECT)
	install -d $(LIBDIR)
	install -m 644 $< $@

$(OBJECT): src/linestream.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(READBUFFER_OBJECT): ../readbuffer/src/readbuffer.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_OBJECT): tests/test_linestream.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_BINARY): $(TEST_OBJECT) $(OBJECT) $(READBUFFER_OBJECT)
	$(CC) $(CFLAGS) $^ -o $@

test: $(TEST_BINARY)
	./$(TEST_BINARY) tests/test_linestream.c

clean:
	rm -rf $(BUILDDIR)
