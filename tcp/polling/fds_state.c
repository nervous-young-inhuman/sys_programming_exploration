#include "fds_state.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int fd_state__watch(Fds *state, int fd) {
	if (state->length >= state->capacity) {
		return -1; // NOT IMPLEMENTING THE REALLOCATION LOGIC
	}

	state->fds[state->length++] =
		(struct pollfd){.fd = fd, .events = POLLIN | POLLOUT, .revents = 0};
	return 0;
}

Fds* fds_state__init() {
        Fds *fdstate = malloc(sizeof(*fdstate));
        if (!fdstate) {
		err_exit("malloc()");
        }
	fdstate->capacity = DEFAULT_FDS_CAPACITY;
	fdstate->length = 0;
        fdstate->fds = malloc(sizeof(struct pollfd) * fdstate->capacity);
	if (!fdstate->fds) {
		free(fdstate);
		err_exit("malloc");
	}
        memset(fdstate->fds, 0, sizeof(struct pollfd) * fdstate->capacity);
	return fdstate;
}

void fds_state__destroy(Fds *fdstate) {
	if (fdstate) {
		if (fdstate->fds)
			free(fdstate->fds);
		free(fdstate);
	}
}
