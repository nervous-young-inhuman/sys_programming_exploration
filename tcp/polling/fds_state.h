#ifndef FDS_STATE_H
#define FDS_STATE_H

#include <sys/poll.h>
#include <stddef.h>

#define DEFAULT_FDS_CAPACITY (64 * 1024) // for 64K fd tracking, we'll need around 512K of memory. 512K = 64K * sizeof(struct pollfd)

typedef struct fds_state {
	size_t capacity;
	nfds_t length;
	struct pollfd *fds;
} Fds;

int watch_for_fd(Fds *state, int fd);
Fds* fds_state__init();
void fds_state__destroy(Fds *fdstate);

#endif
