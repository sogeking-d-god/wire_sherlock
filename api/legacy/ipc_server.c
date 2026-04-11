#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/wiresherlock.sock"
#define BUFFER_SIZE 1024

int main()
{
    int server_fd, client_socket;
    struct sockaddr_un address;
    char buffer[BUFFER_SIZE] = {0};

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[C Server] Listening on UNIX socket: %s\n", SOCKET_PATH);

    if ((client_socket = accept(server_fd, NULL, NULL)) < 0)
    {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    printf("[C Server] Python client connected via UNIX Socket!\n");

    read(client_socket, buffer, BUFFER_SIZE);
    printf("[C Server] Received: %s\n", buffer);

    const char *json_response = "{\"status\": \"success\", \"message\": \"Fast IPC Ready\"}";
    send(client_socket, json_response, strlen(json_response), 0);

    close(client_socket);
    close(server_fd);
    unlink(SOCKET_PATH);

    return 0;
}