CC ?= cc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -Werror -O2

BUILD_DIR := build
LIB := $(BUILD_DIR)/libreadbuffer.a
LIB_OBJECT := $(BUILD_DIR)/readbuffer.o
TEST := $(BUILD_DIR)/test_readbuffer

.PHONY: all test clean

all: $(LIB)

$(BUILD_DIR):
	mkdir -p $@

$(LIB_OBJECT): src/readbuffer.c include/readbuffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LIB): $(LIB_OBJECT)
	$(AR) rcs $@ $^

$(TEST): tests/test_readbuffer.c $(LIB) include/readbuffer.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) -o $@

test: $(TEST)
	./$(TEST)

clean:
	rm -rf $(BUILD_DIR)
