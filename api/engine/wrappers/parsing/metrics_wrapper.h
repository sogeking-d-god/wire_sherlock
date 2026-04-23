#ifndef METRICS_WRAPPER_H
#define METRICS_WRAPPER_H

#include <cjson/cJSON.h>
#include "parser.h"

void handle_get_bins_request(int client_sock, file_analysis_context_t *core, cJSON *request);

#endif