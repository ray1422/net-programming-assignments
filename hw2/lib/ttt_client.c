#include "ttt_client.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

static char buf[8192];
int ttt_client(char *addr_str, int port) {
    struct sockaddr_in sin = parse_sin(addr_str, port);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    if (connect(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("connect");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "connected\n");
    for (; read(sockfd, buf, 1) > 0;) {
        puts(buf);
    }
}