
#include "ttt_server.h"

#include <string.h>
#include <unistd.h>
#include "utils/hash_map.h"
#define MAX_CLIENTS 8192
static HashTable *clients = NULL;

struct client {
    int game_id;
    int logged_in;
};


int client_new(int fd) {
    if (fd > MAX_CLIENTS) return -1;
    // read username and password
    int n_username;
    read(fd, &n_username, sizeof(n_username));

    return 0;
}
int client_handle(int fd) {}