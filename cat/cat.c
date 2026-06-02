/**
 **cat-lite**
   * Implement a simplified `cat`.
   * Use only `open`, `read`, `write`, `close`.'
   * Handle arbitrarily large files.
**/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define PAGE_SIZE 4096 // assuming 4KiB for easiness, check yours using `getconf PAGESIZE`
#define err_exit(s) do { perror(s); exit(1); } while(0)
#define write_file_fd STDOUT_FILENO

int main(int argc, char *argv[])
{
  if (argc < 2) {
    write(STDOUT_FILENO, "usage: ./cat <file>\n", 21);
    return 1;
  }

  const int read_file_fd = (memcmp(argv[1], "-", 1) == 0)
    ? STDIN_FILENO
    : open(argv[1], O_RDONLY);
  
  char read_buffer[PAGE_SIZE];
  ssize_t read_bytes_size = 0;
  if (read_file_fd == -1) {
    err_exit("open");
  }

  while (1) {
    read_bytes_size = read(read_file_fd, read_buffer, PAGE_SIZE - 1);
    if (read_bytes_size == -1)
      err_exit("read");

    if (read_bytes_size == 0) {/* EOF */
      close(read_file_fd);
      break;
    }
  
    if (read_bytes_size > 0) {
      write(write_file_fd, read_buffer, read_bytes_size);
      memset(read_buffer, '\0', PAGE_SIZE);
    }
  }
  
  return 0;
}
