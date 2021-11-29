#pragma once
#define MAX_CLIENTS 8192
#define MAX_LINE 8192
int ttt_server(char *addr, int port);

int ttt_server_hub(int fd[2]);
int leave_player(int fd);
int client_handle(int fd);
int client_new(int fd);