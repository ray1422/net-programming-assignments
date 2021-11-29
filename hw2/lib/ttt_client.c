#include "ttt_client.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "ttt_action.h"
#include "utils.h"
static int game_loop(int fd, int player_id);
static char buf[8192];

static int gird[3][3];
static void dump_gird() {
    const char GI[] = " OX";
    puts("\033c");
    printf("+---+---+---+\n");
    printf("| %c | %c | %c |\n", GI[gird[0][0]], GI[gird[0][1]], GI[gird[0][2]]);
    printf("+---+---+---+\n");
    printf("| %c | %c | %c |\n", GI[gird[1][0]], GI[gird[1][1]], GI[gird[1][2]]);
    printf("+---+---+---+\n");
    printf("| %c | %c | %c |\n", GI[gird[2][0]], GI[gird[2][1]], GI[gird[2][2]]);
    printf("+---+---+---+\n");
    puts("");
}
static int login(int sockfd) {
    fputs("username: ", stdout);
    char username[8192], password[8192];
    scanf("%8000s", username);
    fputs("password: ", stdout);
    scanf("%8000s", password);

    uint32_t len_username = htonl((uint32_t)strlen(username));
    uint32_t len_password = htonl((uint32_t)strlen(password));
    uint32_t action_login = htonl((uint32_t)ttt_login);
    char *cur = buf;
    memcpy(cur, &action_login, sizeof(action_login));
    cur += sizeof(action_login);
    memcpy(cur, &len_username, sizeof(len_username));
    cur += sizeof(len_username);
    memcpy(cur, username, strlen(username));
    cur += strlen(username);
    memcpy(cur, &len_password, sizeof(len_username));
    cur += sizeof(len_password);
    memcpy(cur, password, strlen(password));
    cur += strlen(password);
    write(sockfd, buf, cur - buf);

    uint32_t login_stat = ttt_login_failed;
    read_uint32_from_net(sockfd, &login_stat);
    switch (login_stat) {
        case ttt_login_failed:
            printf("login failed!\n");
            return -1;
            break;

        case ttt_login_success:
            printf("login success!\n");
            break;

        default:
            printf("something went wrong!\n");
            printf("%u\n", login_stat);
            return -1;
            break;
    }
    return 0;
}

// return: 0 for tie, 1 for win, 2 for lose, < 0 for errors
static int game_loop(int fd, int player_id) {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            gird[i][j] = 0;
        }
    dump_gird();
    puts("waiting...");
    for (;;) {
        uint32_t action;
        read_uint32_from_net(fd, &action);
        switch (action) {
            case ttt_do_step:
                uint32_t act_player;
                read_uint32_from_net(fd, &act_player);
                if (act_player == player_id) {
                    uint32_t x, y;
                    do {
                        dump_gird();
                        printf("Your turn!\nPlease input 'x y' in [0, 3)\n");
                        scanf("%u %u", &x, &y);
                        printf("%u %u\n", x, y);
                    } while (!(x < 3 && y < 3) || (gird[x][y] != 0));
                    uint32_t action_step = htonl(ttt_do_step);
                    int x_bak = x, y_bak = y;
                    x = htonl(x);
                    y = htonl(y);
                    char *cur = buf;
                    memcpy(cur, &action_step, sizeof(action_step));
                    cur += sizeof(action_step);
                    memcpy(cur, &x, sizeof(x));
                    cur += sizeof(x);
                    memcpy(cur, &y, sizeof(y));
                    cur += sizeof(y);
                    write(fd, buf, cur - buf);
                    gird[x_bak][y_bak] = 1;
                    dump_gird();
                    printf("Waiting for other...\n");

                } else {
                    uint32_t x, y;
                    read_uint32_from_net(fd, &x);
                    read_uint32_from_net(fd, &y);
                    if (x > 3 || y > 3) {
                        printf("invalid game step. server bugged.\n");
                        printf("x: %u,  y: %u\n", x, y);
                        return -100;
                    }
                    gird[x][y] = 2;
                    dump_gird();
                }
                break;

            case ttt_win:
                printf("win!\n");
                return 1;
                break;

            case ttt_lose:
                printf("lose!\n");
                return 2;
                break;

            case ttt_tie:
                printf("tie!\n");
                return 0;
                break;

            default:
                break;
        }
    }
}
static int invite(int fd, uint32_t player_id) {
    char *cur = buf;
    player_id = htonl(player_id);
    uint32_t action_invite = htonl(ttt_invite);
    memcpy(cur, &action_invite, sizeof(action_invite));
    cur += sizeof(action_invite);
    memcpy(cur, &player_id, sizeof(player_id));
    cur += sizeof(player_id);
    write(fd, buf, cur - buf);

    uint32_t ret;
    read_uint32_from_net(fd, &ret);
    return ret;
}

// return: 1: accept, 0: deny.
static int invite_request(int fd) {
    uint32_t invitor = 0;
    if (read_uint32_from_net(fd, &invitor) != 0) {
        perror("network error");
        exit(EXIT_FAILURE);
    }
    printf("[\033[33;1;5m*\033[0m] %u invites you for a new game! Do you accept it? (y/n)\n", invitor);
    char answer[8192];
    scanf("%8000s", answer);
    if (!strcmp(answer, "y")) {
        write_uint32_to_net(fd, ttt_invite_accept);
        return 1;
    } else {
        write_uint32_to_net(fd, ttt_invite_deny);
        return 0;
    }
}

// return when the game finished
static int lobby(int fd, uint32_t player_id) {
RESTART:
    uint32_t n_clients = 0, action = 0;
    while (1) {
        write_uint32_to_net(fd, ttt_list_clients);
    PARSE_ACTION:
        if (read_uint32_from_net(fd, &action) != 0) {
            perror("network error");
            exit(EXIT_FAILURE);
        }
        switch (action) {
            case ttt_list_clients:
                if (read_uint32_from_net(fd, &n_clients) != 0) {
                    perror("network error");
                    exit(EXIT_FAILURE);
                }
                if (n_clients == 0) {
                    printf("Nobody online now, waiting...\n");
                    sleep(3);
                } else {
                    printf("There %s %u players: \n", n_clients == 1 ? "is" : "are", n_clients);
                    for (int i = 0; i < n_clients; i++) {
                        uint32_t u = 0;
                        read_uint32_from_net(fd, &u);
                        printf(" %4u", u);
                    }
                    printf(
                        "\nEnter player ID to invite for a game, or waiting for others inviting "
                        "you:\n");
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = fd;
                    int epoll_fd = epoll_create1(0);
                    if (epoll_fd == -1) {
                        perror("epoll_create1");
                        exit(EXIT_FAILURE);
                    }

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
                        perror("epoll_ctl: fd");
                        exit(EXIT_FAILURE);
                    }

                    ev.events = EPOLLIN;
                    ev.data.fd = STDIN_FILENO;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
                        perror("epoll_ctl: stdin");
                        exit(EXIT_FAILURE);
                    }
                    while (1) {
                        int nfds = epoll_wait(epoll_fd, &ev, 1, -1);
                        if (nfds < 1) {
                            perror("epoll_wait");
                            continue;
                        }
                        if (ev.data.fd == STDIN_FILENO) {
                            uint32_t player_id;
                            if (scanf("%u", &player_id) == EOF) {
                                printf("bye\n");
                                exit(0);
                            }
                            int result = invite(fd, player_id);
                            if (!result) {
                                printf("player %u is not online, or in another game already.\n",
                                       player_id);
                                close(epoll_fd);
                                goto RESTART;
                            } else {
                                printf("Waiting for [%u]'s reply..\n", player_id);
                                close(epoll_fd);
                                goto PARSE_ACTION;
                            }
                        } else {
                            close(epoll_fd);
                            goto PARSE_ACTION;
                        }
                    }
                }
                break;
            case ttt_invite:
                if (invite_request(fd)) goto PARSE_ACTION;
                goto RESTART;
                break;
            case ttt_start:
                game_loop(fd, player_id);
                goto RESTART;
                break;
            case ttt_invite_deny:
                printf("[%u] declined your invitation!\n", player_id);
                goto RESTART;
                break;
            default:
                printf("unknown action_id: %u\n", action);
                break;
        }
    }

    return 0;
}
int ttt_client(char *addr_str, int port) {
    int ret;
    struct sockaddr_in sin = parse_sin(addr_str, port);
    int sockfd;
    do {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            return EXIT_FAILURE;
        }
        if (connect(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            perror("connect");
            return EXIT_FAILURE;
        }
        fprintf(stderr, "connected\n");
        ret = login(sockfd);
        if (ret != 0) {
            close(sockfd);
            printf("trying to reconnect..\n");
        }
    } while (ret);
    uint32_t player_id = 0;
    read_uint32_from_net(sockfd, &player_id);
    printf("player ID: %u\n", player_id);
    lobby(sockfd, player_id);

    return 0;
}
