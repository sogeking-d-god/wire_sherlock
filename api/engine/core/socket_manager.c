#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "socket_manager.h"

int create_unix_socket(const char *socket_path)
 {
    int server_fd;
    struct sockaddr_un address;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("[SOCKET_MANAGER] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Define the socket address structure
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path, sizeof(address.sun_path) - 1);

    // Remove any existing socket file at the path
    unlink(socket_path);

    // Bind the socket to the specified path
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("[SOCKET_MANAGER] Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_BACKLOG) == -1) {
        perror("[SOCKET_MANAGER] Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

int accept_client(int server_fd)
{
    int client_sock = accept(server_fd, NULL, NULL);
    if (client_sock == -1)
    {
        perror("[SOCKET_MANAGER] Accept failed");
    }
    return client_sock;
}

void close_socket(int fd, const char *socket_path)
{
    if (fd >= 0)
    {
        close(fd);
    }
    if (socket_path != NULL)
    {
        unlink(socket_path);
    }
}