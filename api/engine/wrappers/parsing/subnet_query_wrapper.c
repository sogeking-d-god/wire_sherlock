#include <arpa/inet.h>
#include <string.h>

#include "subnet_query_wrapper.h"
#include "ipc_config.h"
#include "ipc_messenger.h"
#include "ip_tree.h"
#include "flow_table.h"
#include "common.h"

static void subnet_collect_ip_callback(ip_tree_node_t *node,
                                       ip_version_e ip_addr_type,
                                       void *context)
{
    if (!node || !context || !node->ip) return;

    cJSON *ip_arr = (cJSON *)context;
    char ip_buf[INET6_ADDRSTRLEN] = {0};

    get_ip_str((const ip_addr_t *)node->ip, ip_addr_type, ip_buf, sizeof(ip_buf));

    cJSON *ip_obj = cJSON_CreateObject();
    if (!ip_obj) return;

    cJSON_AddStringToObject(ip_obj, "ip", ip_buf);
    cJSON_AddNumberToObject(ip_obj, "packets", (double)node->stats.total_packets);
    cJSON_AddNumberToObject(ip_obj, "bytes",   (double)node->stats.total_bytes);

    if (!cJSON_AddItemToArray(ip_arr, ip_obj))
    {
        cJSON_Delete(ip_obj);
    }
}

void handle_get_subnet_ips_request(int client_sock,
                                   file_analysis_context_t *core,
                                   cJSON *request)
{
    if (!core)
    {
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("No analysis context — call cmd_start first"));
        return;
    }

    cJSON *subnet_item     = cJSON_GetObjectItem(request, "subnet");
    cJSON *prefix_len_item = cJSON_GetObjectItem(request, "prefix_len");
    cJSON *ip_type_item    = cJSON_GetObjectItem(request, "ip_type");

    if (!cJSON_IsString(subnet_item) || !subnet_item->valuestring ||
        !cJSON_IsNumber(prefix_len_item) ||
        !cJSON_IsNumber(ip_type_item))
    {
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("Missing or malformed subnet/prefix_len/ip_type"));
        return;
    }

    int ip_type    = ip_type_item->valueint;
    int prefix_len = prefix_len_item->valueint;

    if (ip_type != 4 && ip_type != 6)
    {
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("ip_type must be 4 or 6"));
        return;
    }

    int max_prefix = (ip_type == 4) ? 32 : 128;
    if (prefix_len < 0 || prefix_len > max_prefix)
    {
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("prefix_len out of range for ip_type"));
        return;
    }

    uint8_t subnet_bytes[IPV6_BYTES] = {0};
    int af = (ip_type == 4) ? AF_INET : AF_INET6;
    if (inet_pton(af, subnet_item->valuestring, subnet_bytes) != 1)
    {
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("subnet is not a valid IP address"));
        return;
    }

    ip_tree_t *tree = (ip_type == 4) ? core->ipv4_tree : core->ipv6_tree;
    cJSON *results = cJSON_CreateArray();

    if (tree && tree->root)
    {
        ip_tree_ret_codes_e rc = ip_tree_find_subnet(tree,
                                                     subnet_bytes,
                                                     (uint8_t)prefix_len,
                                                     subnet_collect_ip_callback,
                                                     results);
        if (rc != IP_TREE_RET_SUCCESS)
        {
            cJSON_Delete(results);
            send_api_response(client_sock, STATUS_ERROR,
                              cJSON_CreateString("ip_tree_find_subnet failed"));
            return;
        }
    }

    send_api_response(client_sock, STATUS_SUCCESS, results);
}
