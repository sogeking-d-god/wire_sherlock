#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cjson/cJSON.h>
#include <arpa/inet.h>

#include "ipc_config.h"
#include "parser.h"
#include "parser_wrapper.h"

void send_json(int socket_fd, cJSON *json_obj)
{
    char *json_str = cJSON_PrintUnformatted(json_obj);
    uint32_t len,net_len;

    if (json_str)
    {
        len = strlen(json_str);
        net_len = htonl(len);

        send(socket_fd, &net_len, sizeof(net_len), 0);

        send(socket_fd, json_str, len, 0);

        free(json_str);
    }

}

cJSON* receive_json(int socket_fd)
{
    uint32_t net_len = 0;
    cJSON *json_obj = NULL;
    uint32_t len;
    char *buffer;

    int bytes_read = recv(socket_fd, &net_len, sizeof(net_len), MSG_WAITALL);

    if (bytes_read > 0)
    {
        len = ntohl(net_len);

        buffer = malloc(len + 1);
        if (buffer)
        {
            bytes_read = recv(socket_fd, buffer, len, MSG_WAITALL);
            if (bytes_read > 0)
            {
                buffer[len] = '\0';

                json_obj = cJSON_Parse(buffer);
            }
            free(buffer);
        }
    }
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

int main(int argc, char *argv[])
{
    char *pcap_path = NULL;
    char *socket_path = NULL;

    int server_fd, client_sock;
    struct sockaddr_un address;

    file_analysis_context_t *core = NULL;
    parser_return_codes_e parse_result;
    cJSON *results_json;

    int running = 1;

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
        cJSON *request = receive_json(client_sock);

        if (request == NULL)
        {
            running = 0;
        }
        else
        {
            cJSON *cmd = cJSON_GetObjectItem(request, "cmd");

            if (cJSON_IsString(cmd) && (cmd->valuestring != NULL))
            {
                if (strcmp(cmd->valuestring, CMD_START_ANALYSIS) == 0)
                {
                    if (core)
                    {
                        core_free(core);
                    }
                    core = malloc(sizeof(file_analysis_context_t));
                    memset(core, 0, sizeof(file_analysis_context_t));

                    // temp direct call to parse, will be changed to call to parser wrapper that will return JSON and handle the retval codes
                    parse_result = parse_pcap_file(core, pcap_path);

                    if(parse_result == PARSER_SUCCESS)
                    {
                        printf("[C_WORKER] Parsing complete. Starting JSON wrap...\n");

                        results_json = wrap_parser_results(core);

                        printf("[C_WORKER] JSON wrap finished successfully. Sending to Python...\n");

                        send_api_response(client_sock, STATUS_SUCCESS, results_json);

                        printf("[C_WORKER] Response sent!\n");
                    }
                    else
                    {
                        results_json = cJSON_CreateString("Error during parsing.");
                        send_api_response(client_sock, STATUS_ERROR, results_json);
                    }
                }

                else if (strcmp(cmd->valuestring, CMD_PING) == 0)
                {
                    send_api_response(client_sock, STATUS_SUCCESS, cJSON_CreateString("Pong! Length-Prefixed Worker is alive."));
                }
                else if (strcmp(cmd->valuestring, CMD_EXIT) == 0)
                {
                    core_free(core);

                    send_api_response(client_sock, STATUS_SUCCESS, cJSON_CreateString("Shutting down."));
                    running = 0;
                }
                else
                {
                    send_api_response(client_sock, STATUS_ERROR, cJSON_CreateString("Unknown command."));
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


// int main() {
//     printf("--- STARTING DRY RUN TEST ---\n");

//     // 1. הקצאת ה-core
//     file_analysis_context_t *core = calloc(1, sizeof(file_analysis_context_t));
//     if (!core) {
//         printf("Failed to allocate core\n");
//         return 1;
//     }

//     // 2. הרצת הפארסר על קובץ pcap אמיתי שיש לך בתיקייה (שים לב לשם הקובץ!)
//     // שנה את "test.pcap" לשם של הקובץ שלך אם הוא שונה
//     parse_pcap_file(core, "../../pcap_files/regular_pcap_file.pcap");

//     // 3. שחרור הזיכרון המיוחל
//     core_free(core);

//     printf("--- DRY RUN FINISHED ---\n");
//     return 0;
// }