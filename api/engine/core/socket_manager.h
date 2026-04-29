#ifndef SOCKET_MANAGER_H
#define SOCKET_MANAGER_H

#define MAX_BACKLOG 5


int create_unix_socket(const char *socket_path);


int accept_client(int server_fd);


void close_socket(int fd, const char *socket_path);

#endif