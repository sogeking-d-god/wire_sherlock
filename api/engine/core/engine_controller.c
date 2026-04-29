#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine_controller.h"

#include "ipc_messenger.h"
#include "ipc_config.h"
#include "parser_wrapper.h"
#include "metrics_wrapper.h"

void run_engine_loop(int client_sock, const char *pcap_path)
{
    file_analysis_context_t *core = NULL;
    int running = 1;

    printf("[Controller] Engine loop started. Ready for commands.\n");

    while (running)
    {
        cJSON *request = receive_json(client_sock);
        if (!request)
        {
            printf("[Controller] Client disconnected or protocol error.\n");
            running = 0;
        }
        else
        {
            cJSON *cmd = cJSON_GetObjectItem(request, "cmd");
            if (cJSON_IsString(cmd) && cmd->valuestring)
            {

                // 1. PING
                if (strcmp(cmd->valuestring, CMD_PING) == 0)
                {
                    send_api_response(client_sock, STATUS_SUCCESS, cJSON_CreateString("Pong from L3 Controller!"));
                }

                // 2. START_ANALYSIS
                else if (strcmp(cmd->valuestring, CMD_START_ANALYSIS) == 0)
                {
                    printf("[Controller] Starting analysis for: %s\n", pcap_path);

                    if (core)
                    {
                        core_free(core);
                    }
                    core = calloc(1, sizeof(file_analysis_context_t));

                    if (parse_pcap_file(core, pcap_path) == 0)
                    {
                        cJSON *results = wrap_parser_results(core);
                        send_api_response(client_sock, STATUS_SUCCESS, results);
                        printf("[Controller] Analysis complete and sent to Python.\n");
                    }
                    else
                    {
                        send_api_response(client_sock, STATUS_ERROR, cJSON_CreateString("Failed to parse PCAP"));
                    }
                }

                // 3. GET_BINS
                else if (strcmp(cmd->valuestring, CMD_GET_BINS) == 0)
                {
                    if (core)
                    {
                        handle_get_bins_request(client_sock, core, request);
                    }
                    else
                    {
                        send_api_response(client_sock, STATUS_ERROR, cJSON_CreateString("No analysis context available"));
                    }
                }

                // 4. EXIT
                else if (strcmp(cmd->valuestring, CMD_EXIT) == 0)
                {
                    send_api_response(client_sock, STATUS_SUCCESS, cJSON_CreateString("Shutting down..."));
                    running = 0;
                }
            }
            cJSON_Delete(request);
        }
    }

    if (core)
    {
        core_free(core);
    }
    printf("[Controller] Engine loop finished.\n");
}