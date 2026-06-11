CC ?= gcc

COMMONS_DIR := /home/sham/af/dirty/sys_programming_exploration/commons

RELEASE ?= 0

BUILD_DIR := build

ifeq ($(RELEASE),1)
BUILD_DIR := $(BUILD_DIR)/release

CFLAGS := \
	-std=c99 \
	-Wall \
	-Wextra \
	-Wpedantic \
	-O2

LDFLAGS :=
else
BUILD_DIR := $(BUILD_DIR)/debug

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
endif

PORT ?= 49497

# LDLIBS := \
#	$(COMMONS_DIR)/lib/linestream.o \
#	$(COMMONS_DIR)/lib/readbuffer.o

TARGET := $(BUILD_DIR)/tcp_pthread

SRCS := tcp_pthread.c
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

run: all
	$(TARGET) $(PORT)

.PHONY: all clean run