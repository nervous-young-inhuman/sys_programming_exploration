#include "readbuffer.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void assert_ok(ErrorM result)
{
    assert(result.is_present == R_OK);
}

static void test_initial_state(void)
{
    ReadBuffer *rb = readbuffer__init(4);
    String view;

    assert(rb != NULL);
    assert(readbuffer__capacity(rb) == 4);
    assert(readbuffer__get_write_cursor_limit(rb) == 4);

    view = readbuffer__get_read_view(rb);
    assert(view.size == 0);
    assert(view.cursor != NULL);

    readbuffer__destroy(rb);
}

static void test_write_and_read_view(void)
{
    ReadBuffer *rb = readbuffer__init(8);
    String view;

    assert(rb != NULL);
    memcpy(readbuffer__get_write_cursor(rb), "hello", 5);
    assert_ok(readbuffer__commit_write(rb, 5));

    view = readbuffer__get_read_view(rb);
    assert(view.size == 5);
    assert(memcmp(view.cursor, "hello", 5) == 0);
    assert(readbuffer__get_write_cursor_limit(rb) == 3);

    readbuffer__destroy(rb);
}

static void test_compaction(void)
{
    ReadBuffer *rb = readbuffer__init(8);
    String view;

    assert(rb != NULL);
    memcpy(readbuffer__get_write_cursor(rb), "abcdef", 6);
    assert_ok(readbuffer__commit_write(rb, 6));
    assert_ok(readbuffer__consume(rb, 4));

    assert(readbuffer__get_write_cursor_limit(rb) == 2);
    assert_ok(readbuffer__assure(rb, 5));
    assert(readbuffer__capacity(rb) == 8);
    assert(readbuffer__get_write_cursor_limit(rb) == 6);

    view = readbuffer__get_read_view(rb);
    assert(view.size == 2);
    assert(memcmp(view.cursor, "ef", 2) == 0);

    memcpy(readbuffer__get_write_cursor(rb), "ghijk", 5);
    assert_ok(readbuffer__commit_write(rb, 5));
    view = readbuffer__get_read_view(rb);
    assert(view.size == 7);
    assert(memcmp(view.cursor, "efghijk", 7) == 0);

    readbuffer__destroy(rb);
}

static void test_power_of_two_growth(void)
{
    ReadBuffer *rb = readbuffer__init(4);

    assert(rb != NULL);
    assert_ok(readbuffer__assure(rb, 90));
    assert(readbuffer__capacity(rb) == 128);
    assert(readbuffer__get_write_cursor_limit(rb) == 128);

    readbuffer__destroy(rb);
}

static void test_growth_preserves_data(void)
{
    ReadBuffer *rb = readbuffer__init(4);
    String view;

    assert(rb != NULL);
    memcpy(readbuffer__get_write_cursor(rb), "abcd", 4);
    assert_ok(readbuffer__commit_write(rb, 4));
    assert_ok(readbuffer__consume(rb, 1));
    assert_ok(readbuffer__assure(rb, 6));

    assert(readbuffer__capacity(rb) == 16);
    view = readbuffer__get_read_view(rb);
    assert(view.size == 3);
    assert(memcmp(view.cursor, "bcd", 3) == 0);
    assert(readbuffer__get_write_cursor_limit(rb) >= 6);

    readbuffer__destroy(rb);
}

static void test_zero_capacity_and_bounds(void)
{
    ReadBuffer *rb = readbuffer__init(0);
    ErrorM result;

    assert(rb != NULL);
    assert(readbuffer__get_write_cursor(rb) == NULL);
    assert_ok(readbuffer__assure(rb, 1));
    assert(readbuffer__capacity(rb) == 1);

    result = readbuffer__commit_write(rb, 2);
    assert(result.is_present == R_ERR);
    assert(result.as.error.code == READBUFFER_ERROR_OUT_OF_RANGE);

    result = readbuffer__consume(rb, 1);
    assert(result.is_present == R_ERR);
    assert(result.as.error.code == READBUFFER_ERROR_OUT_OF_RANGE);

    result = readbuffer__assure(rb, SIZE_MAX);
    assert(result.is_present == R_ERR);
    assert(result.as.error.code == READBUFFER_ERROR_OVERFLOW);

    readbuffer__destroy(rb);
}

int main(void)
{
    test_initial_state();
    test_write_and_read_view();
    test_compaction();
    test_power_of_two_growth();
    test_growth_preserves_data();
    test_zero_capacity_and_bounds();

    puts("readbuffer tests passed");
    return 0;
}
