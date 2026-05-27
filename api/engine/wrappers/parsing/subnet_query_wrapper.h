#ifndef SUBNET_QUERY_WRAPPER_H
#define SUBNET_QUERY_WRAPPER_H

#include <cjson/cJSON.h>
#include "parser.h"

/**
 * IPC handler for cmd_get_subnet_ips.
 *
 * Request JSON:
 *   { "cmd": "cmd_get_subnet_ips",
 *     "subnet": "<dotted-or-colon string>",
 *     "prefix_len": <0..32 for v4, 0..128 for v6>,
 *     "ip_type": 4 | 6 }
 *
 * Response data is a JSON array of { ip, packets, bytes } objects for every
 * leaf in the appropriate global IP tree whose stored address falls inside
 * the requested CIDR block.
 */
void handle_get_subnet_ips_request(int client_sock,
                                   file_analysis_context_t *core,
                                   cJSON *request);

#endif
