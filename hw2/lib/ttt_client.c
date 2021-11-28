#include "ttt_client.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ttt_action.h"
#include "utils.h"

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
    game_loop(sockfd, player_id);
    return 0;
}
