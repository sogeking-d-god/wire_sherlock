#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cjson/cJSON.h>

#include "ipc_config.h"

/**
 * @brief Sends a JSON response to the client
 *
 * @param client_sock  The socket file descriptor for the client connection
 * @param status       The status message
 * @param message      The detailed message
 */
void send_response(int client_sock, const char *status, const char *message)
{
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", status);
    cJSON_AddStringToObject(resp, "message", message);

    char *json_str = cJSON_PrintUnformatted(resp);
    send(client_sock, json_str, strlen(json_str), 0);

    free(json_str);
    cJSON_Delete(resp);
}

int main(int argc, char *argv[])
{
    char *pcap_path = NULL;
    char *socket_path = NULL;

    int server_fd, client_sock;
    struct sockaddr_un address;
    char buffer[BUFFER_SIZE];

    int running = 1;
    int bytes_read = 0;

    // Read args from python
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--pcap") == 0 && i + 1 < argc)
        {
            pcap_path = argv[++i];
        }
        else if (strcmp(argv[i], "--socket") == 0 && i + 1 < argc)
        {
            socket_path = argv[++i];
        }
    }

    if (!pcap_path || !socket_path)
    {
        fprintf(stderr, "Usage: ./c_worker --pcap <file> --socket <path>\n");
        exit(EXIT_FAILURE);
    }

    // TO-DO: add call to C engine parsing function with pcap_path as argument



    // Connect to the UNIX socket specified by the Python session manager
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path, sizeof(address.sun_path) - 1);

    unlink(socket_path);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, MAX_BACKLOG);

    // Wait for the Python client to connect
    client_sock = accept(server_fd, NULL, NULL);

    while (running)
    {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = recv(client_sock, buffer, BUFFER_SIZE, 0);

        if (bytes_read <= 0)
        {
            running = 0; // Client disconnected or error occurred
        }
        else
        {
            cJSON *request = cJSON_Parse(buffer);
            cJSON *cmd = cJSON_GetObjectItem(request, "cmd");

            if (cJSON_IsString(cmd) && (cmd->valuestring != NULL))
            {
                if (strcmp(cmd->valuestring, CMD_PING) == 0)
                {
                    send_response(client_sock, STATUS_SUCCESS, "Pong! Worker is alive and holding PCAP.");
                }
                else if (strcmp(cmd->valuestring, CMD_EXIT) == 0)
                {
                    send_response(client_sock, STATUS_SUCCESS, "Shutting down.");
                    running = 0;
                }
                else
                {
                    send_response(client_sock, STATUS_ERROR, "Unknown command.");
                }
            }
            cJSON_Delete(request);
        }
    }

    close(client_sock);
    close(server_fd);
    unlink(socket_path);

    return 0;
}