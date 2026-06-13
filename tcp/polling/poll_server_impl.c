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

/**
 * Multiple clients can connect.
 * Messages from one client are broadcast to all others.
 * Use `poll()` as the event loop.
 **/
int poll_server_impl(int server_fd) {
	struct sockaddr_storage incoming_addr;
	socklen_t incoming_addr_len = sizeof(incoming_addr);
  
	Fds *fdstate = fds_state__init();
	int rv = fd_state__watch(fdstate, server_fd);
	if (rv == -1) {
		fds_state__destroy(fdstate);
		return -1;
	}

	struct timespec timeout = {
		.tv_sec = 2,
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
			int server_event = fdstate->fds[0].revents;
			if (server_event & POLLIN) {
				int client_fd = accept(server_fd, (struct sockaddr *)&incoming_addr,
						       &incoming_addr_len);

				rv = fd_state__watch(fdstate, client_fd);
				if (rv == -1) {
					barf("failed to add client to watch list");
				}
			}

			if (server_event & POLLNVAL) { /* handle fd not open*/
				barf("oh no, server went bad ugh!!, lets also go out");
				break;
			}

			if (server_event & (POLLHUP | POLLRDHUP)) {
				barf("POLLHUP | POLLRDHUP.. let's disconnect");
				break;
			}


                        sds client_messages = sdsempty();
                            
			/* handle client events */
                        for (nfds_t i = 1; i < fdstate->length; i++) {
			   struct pollfd evt = fdstate->fds[i];
			   if (evt.revents & POLLIN) {
				   client_messages = sdscatfmt(client_messages, "\n<%i>:", (evt.fd));
				   size_t oldlen = sdslen(client_messages);
				   client_messages =
					   sdsMakeRoomFor(client_messages, 1024);

                                   size_t recv_bytes = recv(evt.fd, client_messages + oldlen, 1024, 0);

                                   if (recv_bytes) {
                                     if (client_messages[oldlen + recv_bytes - 1] == '\n')
					     recv_bytes -= 1;
				     sdsIncrLen(client_messages, recv_bytes);
				   }
                           }
                        }

                        if (sdslen(client_messages)) {
                          for (nfds_t i = 1; i < fdstate->length; i++) {
                            struct pollfd evt = fdstate->fds[i];
                            if (evt.revents & POLLOUT) {
                              ssize_t sent_bytes =
                                send(evt.fd, client_messages,
                                     sdslen(client_messages), 0);
                              if (sent_bytes == -1)
                                barf("send()");
                              send(evt.fd, "\n", 1, 0);
                            }
                          }
                        }
			sdsfree(client_messages);
		}
	}

	fds_state__destroy(fdstate);
	return 0;
}
