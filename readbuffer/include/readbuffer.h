#ifndef READBUFFER_H
#define READBUFFER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ReadBuffer ReadBufferT;
typedef ReadBufferT ReadBuffer;

enum ResultStatus {
    R_ERR = 0,
    R_OK = 1
};

typedef struct {
    char message[32];
    unsigned short code;
} Error;

typedef struct {
    union {
        Error error;
    } as;
    enum ResultStatus is_present;
} ErrorM;

typedef struct {
    const char *cursor;
    size_t size;
} String;

enum ReadBufferErrorCode {
    READBUFFER_ERROR_NONE = 0,
    READBUFFER_ERROR_INVALID_ARGUMENT = 1,
    READBUFFER_ERROR_ALLOCATION_FAILED = 2,
    READBUFFER_ERROR_OVERFLOW = 3,
    READBUFFER_ERROR_OUT_OF_RANGE = 4
};

/*
 * Creates an empty buffer. A zero initial capacity is valid and does not
 * allocate storage until readbuffer__assure() requests it.
 */
ReadBuffer *readbuffer__init(size_t init_capacity);

void readbuffer__destroy(ReadBuffer *rb);

/*
 * Ensures that at least requested_space contiguous bytes are available at the
 * write cursor. Existing read data and its order are preserved.
 */
ErrorM readbuffer__assure(ReadBuffer *rb, size_t requested_space);

String readbuffer__get_read_view(const ReadBuffer *rb);
char *readbuffer__get_write_cursor(ReadBuffer *rb);
size_t readbuffer__get_write_cursor_limit(const ReadBuffer *rb);

/*
 * Records bytes written through the write cursor and consumes bytes from the
 * front of the read view.
 */
ErrorM readbuffer__commit_write(ReadBuffer *rb, size_t bytes_written);
ErrorM readbuffer__consume(ReadBuffer *rb, size_t bytes_consumed);

size_t readbuffer__capacity(const ReadBuffer *rb);

#ifdef __cplusplus
}
#endif

#endif
