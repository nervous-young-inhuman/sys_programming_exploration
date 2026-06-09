#include "readbuffer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct ReadBuffer {
    char *data;
    size_t capacity;
    size_t length;
    size_t offset;
};

#define READBUFFER_SUCCESS \
    ((ReadBufferResult){.ok = true})

#define READBUFFER_FAILURE(code, message)                  \
    ((ReadBufferResult){                                   \
        .ok = false,                                       \
        .error_code = (code),                              \
        .error_message = message,                          \
    })

static int next_power_of_two(size_t value, size_t *result)
{
    size_t capacity = 1;

    if (value == 0) {
        *result = 0;
        return 1;
    }

    while (capacity < value) {
        if (capacity > SIZE_MAX / 2) {
            return 0;
        }
        capacity *= 2;
    }

    *result = capacity;
    return 1;
}

ReadBuffer *readbuffer__init(size_t init_capacity)
{
    ReadBuffer *rb = calloc(1, sizeof(*rb));

    if (rb == NULL) {
        return NULL;
    }

    if (init_capacity != 0) {
        rb->data = malloc(init_capacity);
        if (rb->data == NULL) {
            free(rb);
            return NULL;
        }
    }

    rb->capacity = init_capacity;
    return rb;
}

void readbuffer__destroy(ReadBuffer *rb)
{
    if (rb == NULL) {
        return;
    }

    free(rb->data);
    free(rb);
}

ReadBufferResult readbuffer__assure(ReadBuffer *rb, size_t requested_space)
{
    size_t trailing_space;
    size_t available_space;
    size_t required_capacity;
    size_t new_capacity;
    char *new_data;

    if (rb == NULL) {
        return READBUFFER_FAILURE(READBUFFER_ERROR_INVALID_ARGUMENT,
                                  "read buffer is NULL");
    }

    trailing_space = rb->capacity - (rb->offset + rb->length);
    if (requested_space <= trailing_space) {
        return READBUFFER_SUCCESS;
    }

    available_space = rb->capacity - rb->length;
    if (requested_space <= available_space) {
        memmove(rb->data, rb->data + rb->offset, rb->length);
        rb->offset = 0;
        return READBUFFER_SUCCESS;
    }

    if (requested_space > SIZE_MAX - rb->length) {
        return READBUFFER_FAILURE(READBUFFER_ERROR_OVERFLOW,
                                  "requested capacity overflows");
    }
    required_capacity = rb->length + requested_space;

    if (!next_power_of_two(required_capacity, &new_capacity)) {
        return READBUFFER_FAILURE(READBUFFER_ERROR_OVERFLOW,
                                  "requested capacity overflows");
    }

    new_data = malloc(new_capacity);
    if (new_data == NULL) {
        return READBUFFER_FAILURE(READBUFFER_ERROR_ALLOCATION_FAILED,
                                  "allocation failed");
    }

    if (rb->length != 0) {
        memcpy(new_data, rb->data + rb->offset, rb->length);
    }
    free(rb->data);

    rb->data = new_data;
    rb->capacity = new_capacity;
    rb->offset = 0;
    return READBUFFER_SUCCESS;
}

String readbuffer__get_read_view(const ReadBuffer *rb)
{
  if (rb)
    return (String){
      .cursor = rb->data == NULL ? NULL : (rb->data + rb->offset),
      .size = rb->length
    };

  return (String){0};
}

char *readbuffer__get_write_cursor(ReadBuffer *rb)
{
  if (rb == NULL || rb->data == NULL) {
    return NULL;
  }

  return rb->data + rb->offset + rb->length;
}

size_t readbuffer__get_write_cursor_limit(const ReadBuffer *rb)
{
    if (rb == NULL) {
        return 0;
    }

    return rb->capacity - (rb->offset + rb->length);
}

ReadBufferResult readbuffer__commit_write(ReadBuffer *rb, size_t bytes_written)
{
    if (rb == NULL) {
        return READBUFFER_FAILURE(READBUFFER_ERROR_INVALID_ARGUMENT,
                                  "read buffer is NULL");
    }

    if (bytes_written > readbuffer__get_write_cursor_limit(rb)) {
        return READBUFFER_FAILURE(READBUFFER_ERROR_OUT_OF_RANGE,
                                  "write exceeds assured space");
    }

    rb->length += bytes_written;
    return READBUFFER_SUCCESS;
}

ReadBufferResult readbuffer__consume(ReadBuffer *rb, size_t bytes_consumed)
{
    if (rb == NULL) {
        return READBUFFER_FAILURE(READBUFFER_ERROR_INVALID_ARGUMENT,
                                  "read buffer is NULL");
    }

    if (bytes_consumed > rb->length) {
        return READBUFFER_FAILURE(READBUFFER_ERROR_OUT_OF_RANGE,
                                  "consume exceeds readable bytes");
    }

    rb->offset += bytes_consumed;
    rb->length -= bytes_consumed;
    if (rb->length == 0) {
        rb->offset = 0;
    }

    return READBUFFER_SUCCESS;
}

size_t readbuffer__capacity(const ReadBuffer *rb)
{
    return rb == NULL ? 0 : rb->capacity;
}
