#pragma once

int ttt_server(char *addr, int port);

int ttt_server_hub(int fd[2]);

int client_handle(int fd);
int client_new(int fd);