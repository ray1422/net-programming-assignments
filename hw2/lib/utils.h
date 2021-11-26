#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>

static inline struct sockaddr_in parse_sin(char *addr_str, int port) {
    struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    if (!inet_pton(AF_INET, addr_str, &(sin.sin_addr))) {
        struct hostent *hp = gethostbyname(addr_str);
        if (hp == NULL) {
            perror("gethostbyname");
            exit(EXIT_FAILURE);
        }
        sin.sin_addr = *(struct in_addr *)(hp->h_addr);
    }
    if (port > 65535) {
        fprintf(stderr, "bad port number: %d\n", port);
        exit(EXIT_FAILURE);
    } else {
        sin.sin_port = htons(port);
    }

    return sin;
}

static inline int set_nonblocking(int fd) {
    return fcntl(fd, F_SETFL, O_NONBLOCK);
}