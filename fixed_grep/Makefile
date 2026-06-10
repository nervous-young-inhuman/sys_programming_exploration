CC = gcc
COMMONS_DIR = /home/sham/af/dirty/sys_programming_exploration/commons
CFLAGS = -Wall -Wextra -I$(COMMONS_DIR)/include
LDFLAGS = $(COMMONS_DIR)/lib/linestream.o $(COMMONS_DIR)/lib/readbuffer.o
TARGET = fgrep
SRCS = fgrep.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
