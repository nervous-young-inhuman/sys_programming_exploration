#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // write

#define err_exit(s) do { perror(s); exit(1); } while(0)
#define barf(msg) do {							\
		write(STDERR_FILENO, "" msg "", sizeof(msg) - 1);	\
	} while(0)



/* assure that socketfd >= 0 holds  */
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
		exit(1);
        }

	int socketfd = -1;
        for (validaddr = servinfo; validaddr; validaddr = validaddr->ai_next) {
		socketfd = socket(validaddr->ai_family,
				  validaddr->ai_socktype | SOCK_CLOEXEC, // since we are threading, its better to close
				  validaddr->ai_protocol);
                if (socketfd > 0) {
			int yes = 1; // this is required to be an INT!!
			setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
			break;
                }
        }

        if (!validaddr) {
		freeaddrinfo(servinfo);
		barf("failed to assign a valid address");
		exit(EXIT_FAILURE);
        }


        if (bind(socketfd, validaddr->ai_addr, validaddr->ai_addrlen) == -1) {
		freeaddrinfo(servinfo);
		err_exit("bind");
        }

        freeaddrinfo(servinfo);

	if (socketfd < 0) {
                freeaddrinfo(servinfo);
		barf("socketfd < 0. Failed to socket() to valid address");
		exit(EXIT_FAILURE);
        }
	return socketfd;
}


void handle_client_request(int client_socketfd) {
	ssize_t bytes_sent = 0, bytes_recv = 0;
	char recvbuf[4096];

	while (1) {
		memset(recvbuf, 0, sizeof(recvbuf));

		bytes_recv = recv(client_socketfd, recvbuf, sizeof(recvbuf), 0);
		if (bytes_recv == 0) { /* client closed connection */
			break;
		}
		if (bytes_recv == -1) {
			perror("recv");
			break;
		}

		bytes_sent = send(client_socketfd, recvbuf, bytes_recv, 0);
		if (bytes_sent == -1) {
			perror("send");
			break;
		}
	}
}

void fork_server_impl(int server_socketfd, int client_socketfd)
{
	pid_t pid = fork();
        if (pid == 0) { /* child */
		close(server_socketfd);
		handle_client_request(client_socketfd);
		close(client_socketfd);
		_exit(EXIT_SUCCESS);
        }

        if (pid == -1) {
		perror("fork");
        }
        close(client_socketfd);
}


typedef struct client_thread_args {
	int client_socket_fd;
} client_thread_args_t;

static void *thread_worker(void *a) {
	client_thread_args_t *args = (client_thread_args_t *)a;
	int csockfd = args->client_socket_fd;
	free(args);

	handle_client_request(csockfd);
	close(csockfd);
	return NULL;
}

void pthread_impl(int __server_socketfd, int client_socketfd) {
	(void)__server_socketfd;
	client_thread_args_t *args = malloc(sizeof(*args));
	if (!args) {
		perror("malloc");
		return;
	}
	args->client_socket_fd = client_socketfd;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, 64 * 1024);

        pthread_t tid;
        int rc = 0;
	if ((rc = pthread_create(&tid, &attr, thread_worker, args)) != 0) {
          errno = rc;
		perror("pthread_create");
		free(args);
		close(client_socketfd);
	}

	pthread_attr_destroy(&attr);
}

void server_loop(const unsigned short portno1) {
	char portno[6] = {'\0'};
	snprintf(portno, sizeof(portno), "%hu", portno1);

	int socketfd = get_tcp_server_socket(NULL, portno);

	if (listen(socketfd, SOMAXCONN) == -1)
		err_exit("listen");

	while (1) {
		struct sockaddr_storage incoming_addr;
		socklen_t incoming_addr_len = sizeof(incoming_addr);

		int client_socketfd =
			accept(socketfd,
			       (struct sockaddr *)&incoming_addr,
			       &incoming_addr_len);

		if (client_socketfd == -1) {
			perror("accept");
			continue;
		}

		char ipaddr[INET6_ADDRSTRLEN];

		void *addr = NULL;
		if (incoming_addr.ss_family == AF_INET) {
			struct sockaddr_in *s =
				(struct sockaddr_in *)&incoming_addr;
			addr = &s->sin_addr;
		} else if (incoming_addr.ss_family == AF_INET6) {
			struct sockaddr_in6 *s =
				(struct sockaddr_in6 *)&incoming_addr;
			addr = &s->sin6_addr;
		}

		if (addr != NULL) {
			inet_ntop(incoming_addr.ss_family,
				  addr,
				  ipaddr,
				  sizeof(ipaddr));
			printf("incoming connection from %s\n", ipaddr);
		}

		pthread_impl(socketfd, client_socketfd);
	}
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: ./%s <portno>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

        long port = strtol(argv[1], NULL, 10);
        if (!port) {
		err_exit("strtoul: port number not converted to number");
        }
        if (port < 1024 || port > 65535) {
		barf("port number should be 1024 <= port <= 65535");
		exit(EXIT_FAILURE);
        }
	server_loop((unsigned short) port);

	return EXIT_SUCCESS;
}
