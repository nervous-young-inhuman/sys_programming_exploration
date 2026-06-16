#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define err_exit(s) do { perror(s); exit(EXIT_FAILURE); } while(0)
#define barf(msg) do {                                                  \
                write(STDERR_FILENO, "" msg "", sizeof(msg) - 1);       \
        } while(0)

int get_tcp_server_socket(const char *host, const char *port);

#endif



