/**
 * **Memory-Mapped Grep**
   * Search for a string inside a large file using `mmap`.
   * Compare against a `read()`-based version.
   * Measure performance differences.

 * Even though mmap can be performant, its discouraged from usage in production.
 >>>  uses the mmap(2) system call to read input, if possible, instead of the default read(2) system call. In some situations, --mmap yields better performance. However, --mmap can cause undefined behavior (including core dumps) if an input file shrinks while grep is operating, or if an I/O error occurs.
 ref: https://refspecs.linuxfoundation.org/LSB_1.3.0/gLSB/gLSB/grep.html
 **/

/**
   This implementation only bothers to search fixed strings inside the file.
   this is a deliberate choice to not to introduce regexp search and logics.

   Success Criteria:
     1. Can Search through 1GB file, while consuming <100M of memory.
     2. Verification of the output should be done using grep with fixed string search usage. However our program will only output in this format 'lineno:offset:<matching_string>' e.g: "30: hello"
     3. Pattern length should be variable.
 **/

#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include "linestream.h"


#define get_page_size() sysconf(_SC_PAGE_SIZE)
#define err_exit(s) do { perror(s); exit(1); } while(0)
#define write_file_fd STDOUT_FILENO
#define barf(msg) do { \
    write(STDERR_FILENO, "" msg "", sizeof(msg) - 1); \
} while(0)
#define print_usage() barf("usage: ./fgrep <string> <file>\n")
#define KiB (1024)
#define MiB (1024 * 1024)

void defensive_write(const char *buf, const ssize_t n_bytes)
{
  ssize_t bytes_written = 0;
  while (bytes_written < n_bytes) {
    ssize_t res = write(STDOUT_FILENO, buf + bytes_written, n_bytes - bytes_written);
    if (res < 0) err_exit("write");
    bytes_written += res;
  }
}

char* pattern_search(const char *pattern, size_t pattern_len, const char *text, const size_t text_len)
{
  return memmem(text, text_len, pattern, pattern_len);
}

struct running_buffer {
  char* cursor;
  size_t len;
};

void read_and_search_mmap(int search_file_fd, const char *pattern)
{
    size_t pattern_len = strnlen(pattern, 1 * KiB);
    if (pattern_len > (1 * KiB - 1)) {
        barf("pattern length should be less than 1024 characters");
        exit(1);
    }

    struct stat st;
    if (fstat(search_file_fd, &st) < 0) {
        err_exit("fstat()");
    }

    if (st.st_size == 0) {
        return;
    }

    char *file = mmap(
        NULL,
        st.st_size,
        PROT_READ,
        MAP_PRIVATE,
        search_file_fd,
        0);

    if (file == MAP_FAILED) {
        err_exit("mmap()");
    }

    char *cursor = file;
    size_t remaining = st.st_size;

    while (remaining >= pattern_len) {
        char *found = pattern_search(
            pattern,
            pattern_len,
            cursor,
            remaining);

        if (!found) {
            break;
        }

        fwrite(found, 1, pattern_len, stdout);
        fputc('\n', stdout);
        /* write(STDOUT_FILENO, found, pattern_len); */
        /* write(STDOUT_FILENO, "\n", 1); */

        size_t consumed = (found - cursor) + pattern_len;
        cursor += consumed;
        remaining -= consumed;
    }

    munmap(file, st.st_size);
}


void read_and_search(int search_file_fd, const char *pattern)
{
  size_t pattern_len = strnlen(pattern, 1*KiB);
  if (pattern_len > (1*KiB - 1)) {
    barf("pattern length should be less than 1024 characters");
    exit(1);
  }

  // 1KiB reserve to avoid resizing if required for pattern backlog
  const int bytes_to_read = 4 * MiB;
  const size_t buf_len = bytes_to_read + (1 * KiB);
  char *buf = malloc(sizeof(char) * buf_len);
  if (memset(buf, 0, buf_len) == NULL) {
    free(buf);
    barf("failed to memset");
    exit(1);
  }
  int fd = search_file_fd;
  struct running_buffer chunk;
  chunk.cursor = NULL;
  chunk.len = 0;


  while (1) {
    if (chunk.len >= pattern_len) {
      char *found_idx = pattern_search(pattern, pattern_len, chunk.cursor, chunk.len);
      char* new_cursor = found_idx
        ? (found_idx + pattern_len)
        : ((chunk.cursor + chunk.len) - pattern_len + 1);

      chunk.len = (chunk.cursor + chunk.len) - new_cursor;
      chunk.cursor = new_cursor;

      if (found_idx) {
        
        fwrite(found_idx, 1, pattern_len, stdout);
        fputc('\n', stdout);
      }
    } else {
      memmove(buf, chunk.cursor, chunk.len);
      chunk.cursor = buf;
      ssize_t read_bytes = read(fd, buf + chunk.len, bytes_to_read);
      if (read_bytes == 0) { /* EOF */
        free(buf);
        return;
      }

      if (read_bytes < 0) {
        free(buf);
        err_exit("read()");
      }
      chunk.len = chunk.len + read_bytes;
    }
  }
}

void stream_line_and_search(int fd, const char *pattern)
{
  size_t pattern_len = strnlen(pattern, 1*KiB);
  if (pattern_len > (1*KiB - 1)) {
    barf("pattern length should be less than 1024 characters");
    exit(1);
  }
  
  ResultLineStream rl = linestream__create(fd);
  if (!rl.is_ok) {
    err_exit("linestream__create()");
  }
  
  LineStream *ls = rl.as.ls; 

  off_t lineno = 0;
  ResultString rs;
  LSString line;
  while ((rs = linestream__next(ls)).is_ok) {
    line = rs.as.str;
    if (line.size >= pattern_len) {
      char *found_idx = pattern_search(pattern, pattern_len, line.cursor, line.size);
      if (found_idx) {
        // printf("%l %l", lineno + 1, (found_idx - line.cursor));
        fwrite(found_idx, 1, pattern_len, stdout);
        fputc('\n', stdout);
      }
    }
    ++lineno;
  }

  if (rs.as.err.code != LS_ERR_EOF) {
    LSError err = rs.as.err;
    barf("SOMETHING WENT WRONG WHILE READING.. DEETS below\n");
    fprintf(stderr, "code = %d, message = %s", err.code, err.message);
  }
  linestream__destroy(ls);
}


int main(int argc, char *argv[])
{
  if (argc < 3) {
    print_usage();
    exit(1);
  }

  const int fd = open(argv[2], O_RDONLY);
  stream_line_and_search(fd, argv[1]);
  close(fd);

  return 0;
}
