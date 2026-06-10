#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // write

#define err_exit(s) do { perror(s); exit(1); } while(0)
#define barf(msg) do {							\
		write(STDERR_FILENO, "" msg "", sizeof(msg) - 1);	\
	} while(0)

void server_loop(const unsigned short portno1) {
	char portno[6] = {'\0'};
	snprintf(portno, 6, "%hu", portno1);

        struct addrinfo hints, *servinfo, *validaddr;
        memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        hints.ai_protocol = IPPROTO_TCP;

        int status = 0;
	
	if ((status = getaddrinfo(NULL, portno, &hints, &servinfo)) != 0) {
		fprintf(stderr, "gai error: %s\n", gai_strerror(status));
		exit(1);
        }

	int socketfd = -1;
        for (validaddr = servinfo; validaddr; validaddr = validaddr->ai_next) {
          socketfd = socket(validaddr->ai_family,
                            validaddr->ai_socktype,
                            validaddr->ai_protocol);
          if (socketfd > 0) {
		  break;
          }
        }
        if (socketfd < 0) {
                freeaddrinfo(servinfo);
		err_exit("socket failed to connect");
        }

        if (!validaddr) {
          freeaddrinfo(servinfo);
          barf("failed to assign a valid address");
          exit(EXIT_FAILURE);
        }


        if (bind(socketfd, validaddr->ai_addr, validaddr->ai_addrlen) < 0) {
          freeaddrinfo(servinfo);
          err_exit("bind");
        }

        if (listen(socketfd, 1) == -1)
          err_exit("listen");

        struct sockaddr_storage incoming_addr = {0};
        socklen_t incoming_addr_len = sizeof(incoming_addr_len);

        char ipaddr[INET6_ADDRSTRLEN] = {'\0'};
        while (1) {
          int client_socketfd =
            accept(socketfd, (struct sockaddr *)&incoming_addr, &incoming_addr_len);
          if (client_socketfd == -1) {
            freeaddrinfo(servinfo);
            close(socketfd);
            err_exit("accept");
          }

          ssize_t bytes_sent = send(client_socketfd, "hello\n", 6, 0);
            if (bytes_sent == -1) {
              perror("send");
            }
          while (1) {
            char recvbuf[4096] = {'\0'};
            ssize_t bytes_recv =
              recv(client_socketfd, &recvbuf, sizeof(recvbuf), 0);

            if (bytes_recv == 0) { /* client closed connection */
              if (incoming_addr.ss_family == AF_INET) {
                struct sockaddr_in *s = (struct sockaddr_in *)&incoming_addr;
                inet_ntop(AF_INET, &s->sin_addr, ipaddr, sizeof(ipaddr));
              } else if (incoming_addr.ss_family == AF_INET6) {
                struct sockaddr_in6 *s = (struct sockaddr_in6 *)&incoming_addr;
                inet_ntop(AF_INET6, &s->sin6_addr, ipaddr, sizeof(ipaddr));
              }
              fprintf(stdout, "closing connection: %s\n", ipaddr);
              close(client_socketfd);
              break;
            }
            if (bytes_recv == -1) {
              perror("recv");
              break;
            }

            ssize_t bytes_sent = send(client_socketfd, recvbuf, bytes_recv, 0);
            if (bytes_sent == -1) {
              perror("send");
            }
          }
        }
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		barf("usage: ./tcp_single <port>");
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
