#include "linestream.h"
#include "readbuffer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     /* read(2), getpagesize(2) */

/* -------------------------------------------------------------------------
 * Tunables
 * ------------------------------------------------------------------------- */

#define KiB (1024u)
#define MiB (1024u * KiB)

/*
 * Minimum contiguous write space we guarantee before each read(2) call.
 * 4 MiB keeps the number of syscalls low for typical log-file workloads
 * without consuming excessive virtual memory.
 */
#define READ_BUFFERING_SIZE (4u * MiB)

#define get_page_size() sysconf(_SC_PAGE_SIZE)

/* -------------------------------------------------------------------------
 * linestream__create
 * ------------------------------------------------------------------------- */

ResultLineStream linestream__create(int fd)
{
    LineStream *ls = malloc(sizeof(*ls));
    if (!ls) {
        return LINESTREAM_FAILURE(LS_ERR_INIT_FAILED,
                                  "linestream__create: malloc failed");
    }

    ReadBufferT *rb = readbuffer__init(get_page_size());
    if (!rb) {
        free(ls);
        return LINESTREAM_FAILURE(LS_ERR_INIT_FAILED,
                                  "linestream__create: readbuffer__init failed");
    }

    ls->fd          = fd;
    ls->read_buffer = rb;
    return LINESTREAM_OK(ls);
}

/* -------------------------------------------------------------------------
 * linestream__next
 *
 * Design overview
 * ---------------
 * We maintain a single `scanned` offset that grows monotonically within the
 * *current* read view.  When new bytes arrive (after a read(2) call) the
 * view's .size grows but .cursor stays the same (ReadBuffer preserves
 * existing data on assure/commit), so `scanned` correctly resumes scanning
 * from where we left off — avoiding re-inspection of bytes already checked.
 *
 * On success we consume exactly (line_length + 1) bytes from the front of
 * the ReadBuffer (the +1 discards the '\n'), then return a view into the
 * *old* cursor before consumption.  Because the returned LSString.cursor
 * points into ReadBuffer-managed storage that has been consumed, the caller
 * must finish using the string before the next call to linestream__next.
 * This is documented in the header.
 * ------------------------------------------------------------------------- */

ResultString linestream__next(LineStream *ls)
{
    if (!ls) {
        return RESULTSTRING_FAILURE(LS_ERR_NULL_ARG,
                                    "linestream__next: NULL LineStream");
    }

    size_t scanned = 0;

    while (1) {

        /* ---- 1. Inspect what is already in the buffer ---- */

        String rbview = readbuffer__get_read_view(ls->read_buffer);

        if (rbview.size > scanned) {
            char *nl = memchr(rbview.cursor + scanned,
                              '\n',
                              rbview.size - scanned);
            if (nl) {
                size_t      line_len   = (size_t)(nl - rbview.cursor);
                const char *line_start = rbview.cursor;   /* save before consume */

                /*
                 * Consume line + newline so the next call starts fresh.
                 * Cannot fail: we derived line_len from the view itself.
                 */
                readbuffer__consume(ls->read_buffer, line_len + 1);

                return RESULTSTRING_OK(line_start, line_len);
            }

            /* Whole current view examined; remember how far we got. */
            scanned = rbview.size;
        }

        /* ---- 2. Ensure enough contiguous write space ---- */

        ReadBufferResult ar = readbuffer__assure(ls->read_buffer,
                                                 READ_BUFFERING_SIZE);
        if (!ar.ok) {
            return RESULTSTRING_FAILURE(LS_ERR_ALLOC_FAILED,
                                        ar.error_message);
        }

        /* ---- 3. Fill the write region ---- */

        char  *wc    = readbuffer__get_write_cursor(ls->read_buffer);
        size_t limit = readbuffer__get_write_cursor_limit(ls->read_buffer);

        ssize_t n = read(ls->fd, wc, limit);

        if (n < 0) {
            return RESULTSTRING_FAILURE(LS_ERR_READ_FAILED, strerror(errno));
        }

        if (n == 0) {
            /*
             * EOF.  Return residual data (final line with no trailing '\n')
             * if present; otherwise signal clean EOF.
             */
            rbview = readbuffer__get_read_view(ls->read_buffer);
            if (rbview.size > 0) {
                const char *line_start = rbview.cursor;
                size_t      line_len   = rbview.size;
                readbuffer__consume(ls->read_buffer, line_len);
                return RESULTSTRING_OK(line_start, line_len);
            }
            return RESULTSTRING_FAILURE(LS_ERR_EOF,
                                        "linestream__next: end of file");
        }

        /* Commit the bytes that read(2) placed in the write region. */
        ReadBufferResult cr = readbuffer__commit_write(ls->read_buffer,
                                                       (size_t)n);
        if (!cr.ok) {
            return RESULTSTRING_FAILURE(LS_ERR_ALLOC_FAILED,
                                        cr.error_message);
        }

        /*
         * Loop back: the view is now wider by `n` bytes; `scanned` tells
         * memchr where to resume so we don't re-examine old bytes.
         */
    }
}

/* -------------------------------------------------------------------------
 * linestream__destroy
 * ------------------------------------------------------------------------- */

void linestream__destroy(LineStream *ls)
{
    if (!ls) return;
    readbuffer__destroy(ls->read_buffer);
    free(ls);
}
