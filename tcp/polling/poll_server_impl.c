#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sds.h"
#include "fds_state.h"
#include "utils.h"


// client_data_read_ready_handler
// client_data_write_ready_handler
struct client_message {
  int client_id;
  char *message;
};



int message_queue__q(struct message_queue q, struct client_message message);
int handle_client_event(const struct pollfd *event, Fds *fdstate, struct message_queue q) {
  // possible events
  //   client has data ready to be read -> where should it read???
  //   client has data ready to be written -> what should be written here??
  //   client disconnect

  int clientfd = event->fd;
  int revent = event->revents;

  if (revent & POLLIN) {
    sds msgbuf = sdsempty();
    sdsMakeRoomFor(msgbuf, 512);
    struct client_message msg = {
      .client_id = clientfd,
      .message = msgbuf,
    };
    message_queue__q(q, msg);
  }

  /* if (revent & POLLOUT) { */
  /*   ;; // this is almost always triggered */
  /* } */

  if (revent & (POLLHUP | POLLRDHUP)) {
    fd_state__unwatch(fdstate, clientfd);
    close(clientfd);
    return 0;
  }
  return 0;
}

/* -1 server close */
/* -2 failed to watch client */
int handle_server_event(const struct pollfd *event, Fds *fdstate) {
	struct sockaddr_storage incoming_addr;
	socklen_t incoming_addr_len = sizeof(incoming_addr);
	short revents = event->revents;

	if (revents & (POLLERR | POLLNVAL)) {
		/* server socket error, time to close the connection */
		return -1;
	}

	if (revents & POLLIN) {
		int client_fd = accept(event->fd, (struct sockaddr *)&incoming_addr,
				       &incoming_addr_len);
                int rv = fd_state__watch(fdstate, client_fd);
                if (rv == -1)
			return -2;
	}

	if (revents & POLLOUT) {
          barf("server: POLLOUT");
	}

	if (revents & (POLLHUP | POLLRDHUP)) {
          // peer disconnected
          // close connection after processing any remaining reads
              barf("POLLHUP | POLLRDHUP");
        }
	return 0;
}


/**
 * Use poll() as the event loop.
 * Multiple clients can connect and disconnect cleanly.
 * Messages from one client are broadcast to all others.
 * Each client has a queue_offset into a shared ring buffer.
 * Kick clients whose lag exceeds a threshold.
 * POLLOUT is registered only when a client has pending data, deregistered when
 *drained. Pending disconnections are deferred and cleaned up after each
 *broadcast loop.
 **/

/**
 * agreements on what is considered slow
 *  a client can send at max 512bytes of information
 **/
int poll_server_impl(int server_fd) {
	Fds *fdstate = fds_state__init();
	int rv = fd_state__watch(fdstate, server_fd);
	if (rv == -1) {
		fds_state__destroy(fdstate);
		return -1;
	}

	struct timespec timeout = {
		.tv_sec = 1,
		.tv_nsec = 10 * 1000,
	};


	while (1) {
		rv = ppoll(fdstate->fds, fdstate->length, &timeout, NULL);
		if (rv == 0) { /* poll timeout */
			barf("...");
		}
		if (rv == -1) { // something went wrong oops
			perror("ppoll");
			break;
                } else {
			rv = handle_server_event(fdstate->fds, fdstate);
			if (rv == -1) {
				barf("adios amigos, closing server connection");
				break;
			}

			if (rv == -2) {
				barf("failed to add client to watch list");
                        }

                        for (nfds_t i = 1; i < fdstate->length; i++) {
                          rv = handle_client_event(fdstate->fds + i, fdstate);
                        }
		}
	}

	fds_state__destroy(fdstate);
	return 0;
}
