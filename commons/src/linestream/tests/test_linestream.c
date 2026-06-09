#include "linestream.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define barf(msg) do { \
    write(STDERR_FILENO, "" msg "", sizeof(msg) - 1); \
  } while(0)

void test_happy_path(const char *filepath)
{
  int fd = open(filepath, O_RDONLY);
  assert(fd > 0);

  ResultLineStream rl = linestream__create(fd);
  assert(rl.is_ok);
  assert(rl.as.ls != NULL);
  LineStream *ls = rl.as.ls;
  
  ResultString rs;
  while ((rs = linestream__next(ls)).is_ok) {
    // print line
    write(STDOUT_FILENO, rs.as.str.cursor, rs.as.str.size);
    write(STDOUT_FILENO, "\n", 1);
  }

  if (rs.as.err.code != LS_ERR_EOF) {
    LSError err = rs.as.err;
    barf("SOMETHING WENT WRONG WHILE READING.. DEETS below\n");
    fprintf(stderr, "code = %d, message = %s", err.code, err.message);
  }
  linestream__destroy(ls);
}

int main(int argc, const char *argv[])
{
  assert(argc > 1);
  test_happy_path(argv[1]);
  return 0;
}
