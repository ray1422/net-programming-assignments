#pragma once
#include <arpa/inet.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>

static inline struct sockaddr_in parse_sin(char *addr_str, int port) {
    struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));
    if (!inet_pton(AF_INET, addr_str, &(sin.sin_addr))) {
        perror("bad address");
        exit(EXIT_FAILURE);
    }
    if (port > 65535) {
        fprintf(stderr, "bad port number: %d\n", port);
        exit(EXIT_FAILURE);
    } else {
        sin.sin_port = port;
    }

    return sin;
}