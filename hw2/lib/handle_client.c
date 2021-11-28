#include <arpa/inet.h>
#include <assert.h>
#include <gc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ttt_action.h"
#include "ttt_server.h"
#include "user_dat.h"
#include "utils.h"
#include "utils/hash_map.h"
#include "utils/llist.h"

struct client {
    struct game *game;
    int logged_in;
    char username[8192];
};
struct game {
    int game_id;
    int grid[3][3];  // -1 for null, 0 1 for players
    int players[2];  // -1 for blank (waiting)
    int turn;
    struct list_instance list_instance;
};


static struct game *waiting_game = NULL;
struct client clients[MAX_CLIENTS];  // 靜態宣告，C語言沒有 Map 麻煩死。

static int leave_player(int fd) {
    if (fd <= 0) return 0;
    if (clients->game == waiting_game) waiting_game = NULL;  // leave waiting game too.
    clients[fd].logged_in = 0;
    clients[fd].game = NULL;
    strcpy(clients[fd].username, "");
    fprintf(stderr, "close client %d\n", fd);
    return close(fd);
}

static struct game *game_init(struct game *game) {
    if (game == NULL) game = (struct game *)GC_malloc(sizeof(struct game));
    assert(game != NULL);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) game->grid[i][j] = -1;

    return game;
}

static void game_finish(struct game *game, int winner);
// determinate who start first, and send signal to players
static void game_start(struct game *game) {
    game->turn = rand() % 2;
    int ret = 0;
    ret |= write_uint32_to_net(game->players[game->turn], ttt_do_step);
    ret |= write_uint32_to_net(game->players[game->turn], game->players[game->turn]);
    if (ret < 0) {
        game_finish(game, -1);
    }
}

static void game_finish(struct game *game, int winner) {
    if (winner < 0) {
        write_uint32_to_net(game->players[0], ttt_tie);
        write_uint32_to_net(game->players[1], ttt_tie);
    } else if (winner == game->players[0]) {
        write_uint32_to_net(game->players[0], ttt_win);
        write_uint32_to_net(game->players[1], ttt_lose);
    } else {
        write_uint32_to_net(game->players[0], ttt_lose);
        write_uint32_to_net(game->players[1], ttt_win);
    }
    leave_player(game->players[0]);
    leave_player(game->players[1]);
    // free(game); // use GC_malloc so free is no longger needed.
}

// non-zero return for invalid step
static int game_step(struct game *game, uint32_t x, uint32_t y) {
    if (x > 2 || y > 2) return 1;
    if (game->grid[x][y] != -1) return 1;
    write_uint32_to_net(game->players[!game->turn], ttt_do_step);
    write_uint32_to_net(game->players[!game->turn], game->players[game->turn]);
    write_uint32_to_net(game->players[!game->turn], x);
    write_uint32_to_net(game->players[!game->turn], y);
    game->grid[x][y] = game->players[game->turn];
    game->turn = !game->turn;
    return 0;
}

// -1 if still no winner, -2 if tie, else winner's fd
static int game_result(struct game *game) {
    for (int i = 0; i < 3; i++) {
        if (game->grid[i][0] == game->grid[i][1] && game->grid[i][1] == game->grid[i][2]) {
            return game->grid[i][0];
        }
        if (game->grid[0][i] == game->grid[1][i] && game->grid[1][i] == game->grid[2][i]) {
            return game->grid[0][i];
        }
    }
    if (game->grid[0][0] == game->grid[1][1] && game->grid[1][1] == game->grid[2][2]) {
        return game->grid[0][0];
    }
    if (game->grid[0][2] == game->grid[1][1] && game->grid[1][1] == game->grid[2][0]) {
        return game->grid[0][2];
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (game->grid[i][j] == -1) return -1;
        }
    }
    return -2;
}

int client_new(int fd) {
    if (fd > MAX_CLIENTS) {
        close(fd);
        return -1;
    }
    if (clients[fd].logged_in) {
        if (waiting_game->players[0] == fd) {
            waiting_game = NULL;
        }
    }
    clients[fd].logged_in = 0;
    clients[fd].game = NULL;
    return 0;
}

static int login(int fd) {
    if (clients[fd].logged_in) {
        if (waiting_game != NULL && waiting_game->players[0] == fd) {
            waiting_game = NULL;
        }
    }
    char username[8192], password[8192];
    if (read_n_and_string(fd, username, 8192) != -1 &&
        read_n_and_string(fd, password, 8192) != -1) {
        if (!user_check(username, password)) {
            write_uint32_to_net(fd, ttt_login_failed);
            leave_player(fd);
            return -1;
        } else {
            write_uint32_to_net(fd, ttt_login_success);
            write_uint32_to_net(fd, (uint32_t)fd);
            printf("%d logged in\n", fd);
        }
    } else {
        leave_player(fd);
        return 1;
    }
    clients[fd].logged_in = 1;
    // search a game room for this client
    if (waiting_game == NULL) {
        waiting_game = game_init(NULL);
        waiting_game->players[0] = fd;
        clients[fd].game = waiting_game;
    } else {
        waiting_game->players[1] = fd;
        clients[fd].game = waiting_game;
        game_start(waiting_game);
        waiting_game = NULL;
    }
    return 0;
}

static int do_step(int fd) {
    struct game *game = clients[fd].game;
    if (game->players[game->turn] == fd) {
    } else {
        game_finish(game, game->players[game->turn]);
    }
    uint32_t x, y;
    if (read_uint32_from_net(fd, &x) < 0 || read_uint32_from_net(fd, &y) < 0) {  // inavalid step
        game_finish(game, game->players[!game->turn]);
        return 1;
    }
    game_step(game, x, y);
    int result = game_result(game);
    if (result != -1) {
        game_finish(game, result);
        return 1;
    }
    write_uint32_to_net(game->players[game->turn], ttt_do_step);
    write_uint32_to_net(game->players[game->turn], game->players[game->turn]);
    return 0;
}

int client_handle(int fd) {
    for (;;) {
        uint32_t action_id;
        if (read_uint32_from_net(fd, &action_id) < 0) {
            break;
        }
        switch (action_id) {
            case ttt_login:
                login(fd);
                break;
            case ttt_do_step:
                do_step(fd);
            default:
                break;
        }
    }
    return 0;
}