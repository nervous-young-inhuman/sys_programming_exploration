CC ?= gcc
CFLAGS ?= -O2

.PHONY: build test clean

build:
	$(CC) $(CFLAGS) -o cat cat.c

test: build
	head -c $$(( 1024 * 1024 * 1024 )) /dev/urandom | base64 > input_test.txt
	time ./cat input_test.txt | tail -n 3

clean:
	rm -f cat input_test.txt
