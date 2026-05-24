#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "http_attack_wrapper.h"
#include "ipc_config.h"
#include "ipc_messenger.h"
#include "flow_table.h"
#include "http_reassembler.h"
#include "common.h"

static void format_flow_key_str(const flow_key_t *key, char *out, size_t out_len)
{
    char src_buf[INET6_ADDRSTRLEN];
    char dst_buf[INET6_ADDRSTRLEN];
    get_ip_str(&key->src_ip, key->ip_type, src_buf, sizeof(src_buf));
    get_ip_str(&key->dst_ip, key->ip_type, dst_buf, sizeof(dst_buf));
    snprintf(out, out_len, "%s:%u->%s:%u/proto=%u",
             src_buf, key->src_port, dst_buf, key->dst_port, key->protocol);
}

void handle_analyze_http_request(int client_sock, file_analysis_context_t *core)
{
    if (!core || !core->flow_table)
    {
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("No analysis context — call cmd_start first"));
        return;
    }

    uint32_t total_matches = 0;
    uint32_t sessions_with_http = 0;

    cJSON *per_flow = cJSON_CreateArray();
    cJSON *matches_arr = cJSON_CreateArray();

    for (uint32_t bucket_idx = 0; bucket_idx < FLOW_HASH_SIZE; bucket_idx++)
    {
        flow_node_t *flow = core->flow_table->buckets[bucket_idx];
        while (flow != NULL)
        {
            if (flow->key.protocol == IPPROTO_TCP)
            {
                char flow_buf[160];
                format_flow_key_str(&flow->key, flow_buf, sizeof(flow_buf));

                uint32_t flow_total = 0;
                session_node_t *session = flow->first_session;
                while (session != NULL)
                {
                    http_reassembler_session_t *l7s = (http_reassembler_session_t *)session->l7;
                    if (l7s != NULL && l7s->is_http_confirmed == TRUE)
                    {
                        sessions_with_http++;
                        flow_total += l7s->total_attack_matches;
                    }
                    session = session->next;
                }

                if (flow_total > 0)
                {
                    cJSON *entry = cJSON_CreateObject();
                    cJSON_AddStringToObject(entry, "flow_key", flow_buf);
                    cJSON_AddNumberToObject(entry, "match_count", flow_total);
                    cJSON_AddItemToArray(per_flow, entry);
                    total_matches += flow_total;
                }
            }
            flow = flow->next;
        }
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "match_count", total_matches);
    cJSON_AddNumberToObject(data, "http_sessions", sessions_with_http);
    cJSON_AddItemToObject(data, "matches", matches_arr);
    cJSON_AddItemToObject(data, "per_flow", per_flow);

    send_api_response(client_sock, STATUS_SUCCESS, data);
}
