#define _GNU_SOURCE
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
