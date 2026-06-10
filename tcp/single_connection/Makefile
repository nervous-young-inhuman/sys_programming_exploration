CC ?= gcc

COMMONS_DIR := /home/sham/af/dirty/sys_programming_exploration/commons

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
# 	$(COMMONS_DIR)/lib/linestream.o \
# 	$(COMMONS_DIR)/lib/readbuffer.o

TARGET := single_connection_server

SRCS := single_connection_server.c
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

run: all
	./$(TARGET) $(PORT)

.PHONY: all clean
