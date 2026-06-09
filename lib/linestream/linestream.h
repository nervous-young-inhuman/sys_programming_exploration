#ifndef LINESTREAM_H
#define LINESTREAM_H

#include "readbuffer.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Shared primitive types
 * ------------------------------------------------------------------------- */

/*
 * A non-owning view into a region of memory.
 * `cursor` points into ReadBuffer-managed storage — valid only until the
 * next mutating call on the owning LineStream.
 */
typedef struct {
    const char  *cursor;
    size_t       size;
} LSString;

/* Error codes returned through ResultString / ResultLineStream */
typedef enum {
    LS_ERR_NONE             = 0,
    LS_ERR_INIT_FAILED      = 1,   /* readbuffer__init returned NULL        */
    LS_ERR_ALLOC_FAILED     = 2,   /* readbuffer__assure failed             */
    LS_ERR_READ_FAILED      = 3,   /* read(2) returned -1                   */
    LS_ERR_EOF              = 4,   /* clean end-of-file, no more lines      */
    LS_ERR_NULL_ARG         = 5,   /* caller passed NULL where disallowed   */
} LSErrorCode;

typedef struct {
    const char *message;
    LSErrorCode code;
} LSError;

/* -------------------------------------------------------------------------
 * Result types  (tagged-union idiom)
 * ------------------------------------------------------------------------- */

typedef enum {
    R_ERR = 0,
    R_OK  = 1,
} ResultStatus;

typedef struct {
    union {
        LSError  err;
        LSString str;
    } as;
    ResultStatus is_ok;
} ResultString;

/* Forward declaration so ResultLineStream can carry the pointer */
typedef struct LineStream LineStream;

typedef struct {
    union {
        LSError    err;
        LineStream *ls;
    } as;
    ResultStatus is_ok;
} ResultLineStream;

/* -------------------------------------------------------------------------
 * Result construction macros
 * ------------------------------------------------------------------------- */

#define LINESTREAM_OK(ls_ptr)                                               \
    ((ResultLineStream){ .is_ok = R_OK, .as.ls = (ls_ptr) })

#define LINESTREAM_FAILURE(err_code, err_message)                           \
    ((ResultLineStream){                                                     \
        .is_ok  = R_ERR,                                                    \
        .as.err = { .code = (unsigned short)(err_code),                     \
                    .message = err_message },                              \
    })


#define RESULTSTRING_OK(cursor_ptr, size_val)                              \
    ((ResultString){ .is_ok = R_OK, .as.str = { (cursor_ptr), (size_val) } })

#define RESULTSTRING_FAILURE(err_code, err_message)                           \
    ((ResultString){                                                     \
        .is_ok  = R_ERR,                                                    \
        .as.err = { .code = (err_code),                     \
                    .message = err_message },                              \
    })  


/* -------------------------------------------------------------------------
 * LineStream
 * ------------------------------------------------------------------------- */

struct LineStream {
    int          fd;
    ReadBufferT *read_buffer;
};

/*
 * linestream__create
 *
 * Allocates a LineStream backed by a ReadBuffer seeded with one OS page.
 * Returns R_OK + a heap-allocated LineStream on success.
 * Returns R_ERR + a descriptive LSError on failure; no resources are leaked.
 */
ResultLineStream linestream__create(int fd);

/*
 * linestream__next
 *
 * Returns the next '\n'-terminated line as a non-owning LSString view into
 * the ReadBuffer.  The view is valid until the next call to linestream__next
 * or linestream__destroy on the same LineStream.
 *
 * The returned string does NOT include the '\n' character.
 *
 * Returns R_ERR with LS_ERR_EOF when no more data is available.
 * Returns R_ERR with LS_ERR_READ_FAILED / LS_ERR_ALLOC_FAILED on I/O or
 * memory errors.
 */
ResultString linestream__next(LineStream *ls);

/*
 * linestream__destroy
 *
 * Releases all resources owned by *ls and frees the LineStream itself.
 * Safe to call with NULL.
 */
void linestream__destroy(LineStream *ls);

#ifdef __cplusplus
}
#endif

#endif /* LINESTREAM_H */
