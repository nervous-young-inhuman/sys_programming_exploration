# ReadBuffer

**DISCLAIMER: The interface was designed by me, while the code prototype was implemented using codex.**

A small C99 library for buffered reading in system programs. The buffer's
internal data structure is opaque to consumers.

## Build

```sh
make
```

This creates `build/libreadbuffer.a`. Compile a consumer with:

```sh
cc -std=c99 -Iinclude example.c build/libreadbuffer.a -o example
```

Run the test suite with:

```sh
make test
```

## Basic Usage

The normal lifecycle is:

1. Create a buffer with `readbuffer__init()`.
2. Reserve contiguous write space with `readbuffer__assure()`.
3. Write into the cursor returned by `readbuffer__get_write_cursor()`.
4. Record the number of bytes written with `readbuffer__commit_write()`.
5. Inspect unread data with `readbuffer__get_read_view()`.
6. Discard processed bytes with `readbuffer__consume()`.
7. Release the buffer with `readbuffer__destroy()`.

```c
#include "readbuffer.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    const char message[] = "hello";
    ReadBufferT *rb = readbuffer__init(4);
    String readable;
    ReadBufferResult result;

    if (rb == NULL) {
        fprintf(stderr, "could not create read buffer\n");
        return 1;
    }

    result = readbuffer__assure(rb, sizeof(message) - 1);
    if (!result.ok) {
        fprintf(stderr, "assure failed: %s\n", result.error_message);
        readbuffer__destroy(rb);
        return 1;
    }

    memcpy(readbuffer__get_write_cursor(rb), message, sizeof(message) - 1);

    result = readbuffer__commit_write(rb, sizeof(message) - 1);
    if (!result.ok) {
        fprintf(stderr, "commit failed: %s\n", result.error_message);
        readbuffer__destroy(rb);
        return 1;
    }

    readable = readbuffer__get_read_view(rb);
    fwrite(readable.cursor, 1, readable.size, stdout);
    putchar('\n');

    result = readbuffer__consume(rb, readable.size);
    if (!result.ok) {
        fprintf(stderr, "consume failed: %s\n", result.error_message);
        readbuffer__destroy(rb);
        return 1;
    }

    readbuffer__destroy(rb);
    return 0;
}
```

## Reading A File

This example reads a file into the buffer in chunks and writes each chunk to
standard output:

```c
#include "readbuffer.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    const size_t chunk_size = 4096;
    ReadBufferT *rb;
    FILE *input;

    if (argc != 2) {
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        return 2;
    }

    input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror("fopen");
        return 1;
    }

    rb = readbuffer__init(chunk_size);
    if (rb == NULL) {
        fprintf(stderr, "could not create read buffer\n");
        fclose(input);
        return 1;
    }

    for (;;) {
        ReadBufferResult result = readbuffer__assure(rb, chunk_size);
        size_t bytes_read;
        String readable;

        if (!result.ok) {
            fprintf(stderr, "assure failed: %s\n", result.error_message);
            break;
        }

        bytes_read = fread(readbuffer__get_write_cursor(rb),
                           1,
                           chunk_size,
                           input);

        result = readbuffer__commit_write(rb, bytes_read);
        if (!result.ok) {
            fprintf(stderr, "commit failed: %s\n", result.error_message);
            break;
        }

        readable = readbuffer__get_read_view(rb);
        if (fwrite(readable.cursor, 1, readable.size, stdout) != readable.size) {
            perror("fwrite");
            break;
        }

        result = readbuffer__consume(rb, readable.size);
        if (!result.ok) {
            fprintf(stderr, "consume failed: %s\n", result.error_message);
            break;
        }

        if (bytes_read < chunk_size) {
            if (ferror(input)) {
                perror("fread");
            }
            break;
        }
    }

    readbuffer__destroy(rb);
    fclose(input);
    return 0;
}
```

## API Notes

- `readbuffer__assure(rb, n)` guarantees at least `n` contiguous bytes at the
  write cursor. It may compact the current data or grow the allocation.
- `readbuffer__get_write_cursor_limit()` returns the number of bytes currently
  writable without another call to `readbuffer__assure()`.
- `readbuffer__commit_write()` must not exceed the current write cursor limit.
- `readbuffer__consume()` must not exceed the current read view size.
- Cursors and read views are borrowed pointers. Do not retain them after
  calling `readbuffer__assure()` or `readbuffer__destroy()`.
- `readbuffer__init(0)` is valid; storage is allocated on the first non-zero
  assurance request.
