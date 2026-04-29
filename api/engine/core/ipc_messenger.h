#ifndef IPC_MESSENGER_H
#define IPC_MESSENGER_H

#include <cjson/cJSON.h>
#include <stdint.h>

void send_json(int socket_fd, cJSON *json_obj);

cJSON* receive_json(int socket_fd);

void send_api_response(int client_sock, const char *status, cJSON *data_body);

void send_binary(int socket_fd, const void *data, uint32_t len);

#endif