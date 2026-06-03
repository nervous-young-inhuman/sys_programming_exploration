/**
2. **tail -f**
   * Implement a file follower.
   * Detect when new data is appended and print it.
   * No polling loops with sleeps if possible.

   Files are not streams. Learn file offsets, blocking behavior, and how the kernel exposes file growth. |
**/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>

#define PAGE_SIZE 4096 // assuming 4KiB for easiness, check yours using `getconf PAGESIZE`
#define err_exit(s) do { perror(s); exit(1); } while(0)
#define write_file_fd STDOUT_FILENO
#define barf(msg) do { \
    write(STDERR_FILENO, "" msg "", sizeof(msg) - 1); \
} while(0)
#define print_usage() barf("usage: ./tail -f <file>\n")


void defensive_write(const char *buf, const ssize_t n_bytes) {
  ssize_t bytes_written = 0;
  while (bytes_written < n_bytes) {
    ssize_t res = write(STDOUT_FILENO, buf + bytes_written, n_bytes - bytes_written);
    if (res < 0) err_exit("write");
    bytes_written += res;
  }
}

void write_till_eof(int following_file_fd)
{
  char buf[PAGE_SIZE];
  while (1) {
    ssize_t n_read_bytes = read(following_file_fd, buf, PAGE_SIZE);
    if (n_read_bytes == -1)
      err_exit("write_till_eof#read");
    if (n_read_bytes == 0)
      return;
    defensive_write(buf, n_read_bytes);
  }
}

int main(int argc, char *argv[])
{

  if (argc < 3) {
    print_usage();
    return 1;
  }

  if (memcmp(argv[1], "-f", 1) != 0) {
    print_usage();
    return 1;
  }

  if ((memcmp(argv[2], "-", 1) == 0)) {
    barf("only passing actual files is supported right now");
    print_usage();
    return 1;
  }
  
  const char* following_file_path = argv[2];
  const int inotify_fd = inotify_init();
  if (inotify_fd < 0) {
    err_exit("inotify_init");
  }
  const int inotify_wd = inotify_add_watch(inotify_fd, following_file_path, IN_MODIFY);
  if (inotify_wd < 0)
    err_exit("inotify_watch");

  struct stat following_file_stat;
  struct inotify_event ine;

  const int following_file_fd = open(following_file_path, O_RDONLY);
  if (following_file_fd < 0)
    err_exit("open");

  off_t returned_idx = lseek(following_file_fd, 0, SEEK_END);
  if (returned_idx < 0) {
    err_exit("lseek");
  }

  while (1) {
    // assume .name field is not accessed
    int read_bytes = read(inotify_fd, &ine, sizeof(struct inotify_event));
    if (read_bytes < 0)
      err_exit("read");

    if (ine.wd == inotify_wd) {
      if (fstat(following_file_fd, &following_file_stat) == -1)
        err_exit("fstat");

      off_t current_offset = lseek(following_file_fd, 0, SEEK_CUR);
      if (following_file_stat.st_size < current_offset) {
        lseek(following_file_fd, 0, SEEK_SET);
      }
      write_till_eof(following_file_fd);
    }
  }
  return 0;
}

/**
   TODO:
      HANDLE FOR TRUNCATION AND ROTATION.
      see the ./tail -f <name>
      means 'watch' for changes in the <name> file,
      so if a new file gets moved there with the same size then that content should be displayed.
 **/
