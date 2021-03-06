#include "ttt_server.h"

#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ttt_action.h"
#include "utils.h"
#define MAX_EVENTS 10
#include "utils.h"
#define ever \
    ;        \
    ;

static void clean_zombie_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].logged_in) {
            write_uint32_to_net(i, ttt_ping);
        }
    }
}
int ttt_server(char *addr_str, int port) {
    unsigned val = 1;
    struct sockaddr_in sin = parse_sin(addr_str, port);
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    if (bind(listen_sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }

    signal(SIGCHLD, SIG_IGN);  // prevent child zombie

    if (listen(listen_sock, SOMAXCONN) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    set_nonblocking(listen_sock);
    struct epoll_event ev, events[MAX_EVENTS];
    int conn_sock;

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            continue;
        }
        if (rand() % 100 > 30) {
            clean_zombie_clients();
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listen_sock) {  // new incoming connection
                conn_sock = accept(listen_sock, (struct sockaddr *)NULL, NULL);
                if (conn_sock == -1) {
                    perror("accept");
                    continue;
                }
                set_nonblocking(conn_sock);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    continue;
                }
                client_new(conn_sock);
            } else {
                client_handle(events[i].data.fd);
            }
        }
    }
    return 0;
}