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
	-O2 \
	-I$(COMMONS_DIR)/include

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
	-fsanitize=undefined \
	-I$(COMMONS_DIR)/include


LDFLAGS := \
	-fsanitize=address \
	-fsanitize=undefined
endif

PORT ?= 49497

LDLIBS := \
	$(COMMONS_DIR)/lib/sds.o

TARGET := $(BUILD_DIR)/poll_server

SRCS := main.c fds_state.c poll_server_impl.c utils.c message_queue.c
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
