CC          ?= cc
PREFIX      ?= /home/sham/af/dirty/sys_programming_exploration/commons
INCLUDEDIR  := $(PREFIX)/include
LIBDIR      := $(PREFIX)/lib
BUILDDIR    := build

CPPFLAGS    += -I$(INCLUDEDIR)
CFLAGS      ?= -std=c99 -Wall -Wextra -Wpedantic -O2

OBJECT      := $(BUILDDIR)/sds.o
INSTALLED_OBJECT := $(LIBDIR)/sds.o

.PHONY: all install clean

all: install

install: $(INSTALLED_OBJECT)

$(INSTALLED_OBJECT): $(OBJECT)
	install -d $(LIBDIR)
	install -m 644 $< $@

$(OBJECT): sds.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)
