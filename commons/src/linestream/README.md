# linestreamer


## Desc
A simple line iteration library.
not sure why i went down this path, when i could have easily used the existing C helper functions


## Example Usage

void test_happy_path(const char *filepath)
{
  int fd = open(filepath, O_RDONLY);
  assert(fd > 0);

  ResultLineStream rl = linestream__create(fd);
  assert(rl.is_ok);
  LineStream *ls = rl.as.ls;
  
  ResultString rs;
  while ((rs = linestream__next(ls)).is_ok) {
    // print line
    write(STDOUT_FILENO, rs.as.str.cursor, rs.as.str.size);
    write(STDOUT_FILENO, "\n", 1);
  }

  /* EOF is treated as an Error for simple modelling of data */
  if (rs.as.err.code != LS_ERR_EOF) {
    LSError err = rs.as.err;
    barf("SOMETHING WENT WRONG WHILE READING.. DEETS below\n");
    fprintf(stderr, "code = %d, message = %s", err.code, err.message);
  }
  linestream__destroy(ls);
}
