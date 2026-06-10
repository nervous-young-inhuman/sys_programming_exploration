CC ?= gcc

COMMONS_DIR := /home/sham/af/dirty/sys_programming_exploration/commons

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

LDLIBS := \
	$(COMMONS_DIR)/lib/linestream.o \
	$(COMMONS_DIR)/lib/readbuffer.o

TARGET := fgrep

SRCS := fgrep.c
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
