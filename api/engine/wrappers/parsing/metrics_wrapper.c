#include <stdio.h>
#include <sys/socket.h>
#include "metrics_wrapper.h"
#include "ipc_config.h"

extern void send_api_response(int client_sock, const char *status, cJSON *data_body);

void handle_get_bins_request(int client_sock, file_analysis_context_t *core, cJSON *request)
{
    int error_code = 0;

    if (!core || !core->bin_manager)
    {
        error_code = 1;
        send_api_response(client_sock, STATUS_ERROR, cJSON_CreateString("Analysis not started or bin manager is empty."));
    }

    cJSON *metric_id_item = cJSON_GetObjectItem(request, "metric_id");
    if (!error_code && (!metric_id_item || !cJSON_IsNumber(metric_id_item)))
    {
        send_api_response(client_sock, STATUS_ERROR, cJSON_CreateString("Missing or invalid metric_id."));
        error_code = 1;
    }

    int metric_id = metric_id_item->valueint;

    if (!error_code && (metric_id < 0 || metric_id >= METRICS_COUNT))
    {
        send_api_response(client_sock, STATUS_ERROR, cJSON_CreateString("Unknown Metric ID: Value out of protocol range."));
        error_code = 1;
    }

    double *bins_array = core->bin_manager->bins[metric_id];

    if (bins_array == NULL)
    {
        send_api_response(client_sock, STATUS_ERROR, cJSON_CreateString("Metric not active: This metric was not requested during initialization."));
        error_code = 1;
    }

    if (!error_code)
    {

        long total_bins = core->bin_manager->total_bins;
        size_t binary_size = total_bins * sizeof(double);
        double *bins_array = core->bin_manager->bins[metric_id];

        cJSON *meta = cJSON_CreateObject();
        cJSON_AddNumberToObject(meta, "metric_id", metric_id);
        cJSON_AddNumberToObject(meta, "total_bins", total_bins);
        cJSON_AddNumberToObject(meta, "byte_size", binary_size);

        double start_ts_double = core->bin_manager->start_ts.tv_sec + (core->bin_manager->start_ts.tv_usec / 1000000.0);
        cJSON_AddNumberToObject(meta, "start_ts", start_ts_double);
        cJSON_AddNumberToObject(meta, "bin_size_ms", core->bin_manager->bin_size);

        send_api_response(client_sock, STATUS_BINARY, meta);

        ssize_t bytes_sent = send(client_sock, bins_array, binary_size, 0);

        if (bytes_sent != binary_size)
        {
            fprintf(stderr, "[C_WORKER] Warning: Failed to send full binary payload. Sent %zd/%zu bytes\n", bytes_sent, binary_size);
        }
        else
        {
            printf("[C_WORKER] Successfully sent %zu bytes of binary data for metric %d.\n", binary_size, metric_id);
        }
    }
}