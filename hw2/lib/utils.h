#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

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
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// non-zero return if failed
static inline int read_uint32_from_net(int fd, uint32_t *ret) {
    uint32_t n;
    if (read(fd, (char *)&n, sizeof(n)) < 0) {
        return -1;
    }
    n = ntohl(n);
    *ret = n;
    return 0;
}


static inline int write_uint32_to_net(int fd, uint32_t n) {
    n = htonl(n);
    return write(fd, &n, sizeof(n));
}

static inline int read_n_and_string(int fd, char *buf, int max_n) {
    uint32_t n;
    if (read_uint32_from_net(fd, &n) < 0) {
        perror("read");
        return -1;
    }

    if (n > max_n - 1) {
        fprintf(stderr, "string too long\n");
        return -1;
    }
    if (read(fd, buf, n) < 0) {
        fprintf(stderr, "read failed\n");
        return -1;
    }
    buf[n] = '\0';
}