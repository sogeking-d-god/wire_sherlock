#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine_controller.h"

#include "ipc_messenger.h"
#include "ipc_config.h"
#include "parser_wrapper.h"
#include "metrics_wrapper.h"
#include "l7_handler.h"

/**
 * @brief Runs the main engine command loop, processing JSON commands from the client.
 *
 * Listens for incoming commands over the socket and dispatches them to the
 * appropriate handlers. Exits when the client disconnects or sends CMD_EXIT.
 *
 * @param client_sock The connected client socket file descriptor.
 * @param pcap_path Path to the PCAP file to analyse when CMD_START_ANALYSIS is received.
 */
void run_engine_loop(int client_sock, const char *pcap_path)
{
    file_analysis_context_t *core = NULL;
    int running = 1;
    cJSON *request;
    cJSON *cmd;
    cJSON *results;

    printf("[Controller] Engine loop started. Ready for commands.\n");

    while (running)
    {
        request = receive_json(client_sock);

        if (!request)
        {
            printf("[Controller] Client disconnected or protocol error.\n");
            running = 0;
        }
        else
        {
            cmd = cJSON_GetObjectItem(request, "cmd");

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
                        // Milestone 3.1: trigger L7 sweep inline after L4 parse so
                        // we can observe HTTP boundary detection logs. A dedicated
                        // CMD_ANALYZE_L7 command will be added in Milestone 3.3.
                        l7_handler_run(core->flow_table, pcap_path);

                        results = wrap_parser_results(core);
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
