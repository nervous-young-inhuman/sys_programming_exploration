CC ?= gcc

COMMONS_DIR := /home/sham/af/dirty/sys_programming_exploration/commons

BUILD_DIR ?= ./build

PORT ?= 49497
CFLAGS := \
        -std=c99 \
        -Wall \
        -Wextra \
        -Wpedantic \
        -g3 \
        -O0 \
        -fsanitize=address \
        -fsanitize=undefined

LDFLAGS := \
        -fsanitize=address \
        -fsanitize=undefined

# LDLIBS := \
#       $(COMMONS_DIR)/lib/linestream.o \
#       $(COMMONS_DIR)/lib/readbuffer.o

TARGET := $(BUILD_DIR)/tcp_pthread

SRCS := tcp_pthread.c
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(OBJS)

run: all
	$(TARGET) $(PORT)

.PHONY: all clean run