#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils.h"

int poll_server_impl(int server_fd);

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
