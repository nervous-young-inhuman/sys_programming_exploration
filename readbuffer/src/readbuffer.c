#include "readbuffer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ReadBuffer {
    char *data;
    size_t capacity;
    size_t length;
    size_t offset;
};

static ErrorM readbuffer_ok(void)
{
    ErrorM result = {0};
    result.is_present = R_OK;
    return result;
}

static ErrorM readbuffer_error(unsigned short code, const char *message)
{
    ErrorM result = {0};
    result.is_present = R_ERR;
    result.as.error.code = code;
    (void)snprintf(result.as.error.message,
                   sizeof(result.as.error.message),
                   "%s",
                   message);
    return result;
}

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

ErrorM readbuffer__assure(ReadBuffer *rb, size_t requested_space)
{
    size_t trailing_space;
    size_t available_space;
    size_t required_capacity;
    size_t new_capacity;
    char *new_data;

    if (rb == NULL) {
        return readbuffer_error(READBUFFER_ERROR_INVALID_ARGUMENT,
                                "read buffer is NULL");
    }

    trailing_space = rb->capacity - (rb->offset + rb->length);
    if (requested_space <= trailing_space) {
        return readbuffer_ok();
    }

    available_space = rb->capacity - rb->length;
    if (requested_space <= available_space) {
        memmove(rb->data, rb->data + rb->offset, rb->length);
        rb->offset = 0;
        return readbuffer_ok();
    }

    if (requested_space > SIZE_MAX - rb->length) {
        return readbuffer_error(READBUFFER_ERROR_OVERFLOW,
                                "requested capacity overflows");
    }
    required_capacity = rb->length + requested_space;

    if (!next_power_of_two(required_capacity, &new_capacity)) {
        return readbuffer_error(READBUFFER_ERROR_OVERFLOW,
                                "requested capacity overflows");
    }

    new_data = malloc(new_capacity);
    if (new_data == NULL) {
        return readbuffer_error(READBUFFER_ERROR_ALLOCATION_FAILED,
                                "allocation failed");
    }

    if (rb->length != 0) {
        memcpy(new_data, rb->data + rb->offset, rb->length);
    }
    free(rb->data);

    rb->data = new_data;
    rb->capacity = new_capacity;
    rb->offset = 0;
    return readbuffer_ok();
}

String readbuffer__get_read_view(const ReadBuffer *rb)
{
    String view = {0};

    if (rb != NULL) {
        view.cursor = rb->data == NULL ? NULL : rb->data + rb->offset;
        view.size = rb->length;
    }

    return view;
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

ErrorM readbuffer__commit_write(ReadBuffer *rb, size_t bytes_written)
{
    if (rb == NULL) {
        return readbuffer_error(READBUFFER_ERROR_INVALID_ARGUMENT,
                                "read buffer is NULL");
    }

    if (bytes_written > readbuffer__get_write_cursor_limit(rb)) {
        return readbuffer_error(READBUFFER_ERROR_OUT_OF_RANGE,
                                "write exceeds assured space");
    }

    rb->length += bytes_written;
    return readbuffer_ok();
}

ErrorM readbuffer__consume(ReadBuffer *rb, size_t bytes_consumed)
{
    if (rb == NULL) {
        return readbuffer_error(READBUFFER_ERROR_INVALID_ARGUMENT,
                                "read buffer is NULL");
    }

    if (bytes_consumed > rb->length) {
        return readbuffer_error(READBUFFER_ERROR_OUT_OF_RANGE,
                                "consume exceeds readable bytes");
    }

    rb->offset += bytes_consumed;
    rb->length -= bytes_consumed;
    if (rb->length == 0) {
        rb->offset = 0;
    }

    return readbuffer_ok();
}

size_t readbuffer__capacity(const ReadBuffer *rb)
{
    return rb == NULL ? 0 : rb->capacity;
}
