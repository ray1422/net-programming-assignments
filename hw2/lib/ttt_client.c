#include "ttt_client.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ttt_action.h"
#include "utils.h"

static char buf[8192];

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
            return -1;
            break;
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
    getchar();
    getchar();
    read_uint32_from_net(sockfd, &player_id);
    printf("player ID: %u\n", player_id);
}
