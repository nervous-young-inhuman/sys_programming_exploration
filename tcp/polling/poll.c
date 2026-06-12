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
#define DEFAULT_FDS_CAPACITY (64 * 1024) // for 64K fd tracking, we'll need around 512K of memory. 512K = 64K * sizeof(struct pollfd)

#define err_exit(s) do { perror(s); exit(EXIT_FAILURE); } while(0)
#define barf(msg) do {                                                  \
                write(STDERR_FILENO, "" msg "", sizeof(msg) - 1);       \
        } while(0)

/**
 * Helper function to create, bind and listen on a TCP server socket.
 */
int get_tcp_server_socket(const char *host, const char *port) {
	struct addrinfo hints, *servinfo, *validaddr;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_TCP;

	int status = 0;
	if ((status = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "gai error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	int socketfd = -1;
	for (validaddr = servinfo; validaddr; validaddr = validaddr->ai_next) {
		socketfd = socket(validaddr->ai_family,
				  validaddr->ai_socktype | SOCK_CLOEXEC,
				  validaddr->ai_protocol);
		if (socketfd > 0) {
			int yes = 1;
			setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
			break;
		}
	}

	if (!validaddr) {
		freeaddrinfo(servinfo);
		barf("failed to assign a valid address\n");
		exit(EXIT_FAILURE);
	}

	if (bind(socketfd, validaddr->ai_addr, validaddr->ai_addrlen) == -1) {
		freeaddrinfo(servinfo);
		err_exit("bind");
	}

	freeaddrinfo(servinfo);

	if (listen(socketfd, SOMAXCONN) == -1) {
		err_exit("listen");
	}

	return socketfd;
}



typedef struct fds_state {
	size_t capacity;
	nfds_t length;
	struct pollfd *fds;
} Fds;

int watch_for_fd(int fd, Fds *state) {
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


/**
 * Multiple clients can connect.
 * Messages from one client are broadcast to all others.
 * Use `poll()` as the event loop.
 **/
int poll_server_impl(int server_fd) {
	struct sockaddr_storage incoming_addr;
	socklen_t incoming_addr_len = sizeof(incoming_addr);
  
	Fds *fdstate = fds_state__init();
	int rv = watch_for_fd(server_fd, fdstate);
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

				rv = watch_for_fd(client_fd, fdstate);
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

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <port>>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *endptr;
	long portno = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0' || portno <= 1024 || portno >= 65535) {
		barf("Error: port number should be 1024 < port < 65535\n");
		exit(EXIT_FAILURE);
	}

	int server_fd = get_tcp_server_socket(NULL, argv[1]);
	fprintf(stdout, "Chat server started on port %lu\n", portno);
	int ec = poll_server_impl(server_fd);
	close(server_fd);
	return ec;
}
