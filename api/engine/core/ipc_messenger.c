#include "ipc_messenger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void send_json(int socket_fd, cJSON *json_obj)
{
    char *json_str = cJSON_PrintUnformatted(json_obj);
    if (!json_str) return;

    uint32_t len = strlen(json_str);
    uint32_t net_len = htonl(len);

    send(socket_fd, &net_len, sizeof(net_len), 0);
    send(socket_fd, json_str, len, 0);

    free(json_str);
}

cJSON* receive_json(int socket_fd)
{
    uint32_t net_len = 0;
    if (recv(socket_fd, &net_len, sizeof(net_len), MSG_WAITALL) <= 0) return NULL;

    uint32_t len = ntohl(net_len);
    char *buffer = malloc(len + 1);
    if (!buffer) return NULL;

    if (recv(socket_fd, buffer, len, MSG_WAITALL) <= 0) {
        free(buffer);
        return NULL;
    }

    buffer[len] = '\0';
    cJSON *json_obj = cJSON_Parse(buffer);
    free(buffer);
    return json_obj;
}

void send_api_response(int client_sock, const char *status, cJSON *data_body)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", status);

    if (data_body)
    {
        cJSON_AddItemToObject(root, "data", data_body);
    }

    send_json(client_sock, root);
    cJSON_Delete(root);
}

void send_binary(int socket_fd, const void *data, uint32_t len)
{
    send(socket_fd, data, len, 0);
}