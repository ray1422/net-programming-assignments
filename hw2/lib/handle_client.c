#define MAX_CLIENTS 8192

struct client {
    int game_id;
    int logged_in;
};

struct client clients[MAX_CLIENTS];

int client_new(int fd) {
    if (fd > MAX_CLIENTS) return -1;
    return 0;
}
int client_handle(int fd) {}