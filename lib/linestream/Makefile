CC        ?= cc
BUILD_DIR := build

# Space-separated list of extra include paths.
# Override from the command line:
#   make INCLUDE_DIRS="../readbuffer/include ../common/include"
INCLUDE_DIRS ?= ../readbuffer/include

INCLUDES  := $(addprefix -I, $(INCLUDE_DIRS))

CFLAGS    ?= -Iinclude $(INCLUDES) -std=c99 -Wall -Wextra -Wpedantic -Werror -O2

LIB_OBJECT := $(BUILD_DIR)/linestream.o

.PHONY: all clean

all: $(LIB_OBJECT)

$(BUILD_DIR):
	mkdir -p $@

$(LIB_OBJECT): linestream.c linestream.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
