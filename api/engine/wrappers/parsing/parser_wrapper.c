#include <arpa/inet.h>
#include <string.h>

#include "parser_wrapper.h"
#include "flow_table.h"
#include "mac_table.h"

static double get_precise_time(struct timeval *tv)
{
    double ret = 0;
    if (tv)
    {
        ret = (double)tv->tv_sec + ((double)tv->tv_usec / 1000000.0);
    }
    return ret;
}

static void add_flow_to_json_callback(flow_node_t *node, void *context)
{
    cJSON *flows_array = (cJSON *)context;
    cJSON *flow_obj = cJSON_CreateObject();
    cJSON *key_obj = cJSON_CreateObject();
    cJSON *sessions_array = cJSON_CreateArray();
    session_node_t *curr_session;
    char ip_buf[INET6_ADDRSTRLEN];
    int device_idx;
    int session_idx = 0;

    get_ip_str(&node->key.src_ip, node->key.ip_type, ip_buf, sizeof(ip_buf));
    cJSON_AddStringToObject(key_obj, "src_ip", ip_buf);

    get_ip_str(&node->key.dst_ip, node->key.ip_type, ip_buf, sizeof(ip_buf));
    cJSON_AddStringToObject(key_obj, "dst_ip", ip_buf);

    cJSON_AddNumberToObject(key_obj, "src_port", ntohs(node->key.src_port));
    cJSON_AddNumberToObject(key_obj, "dst_port", ntohs(node->key.dst_port));
    cJSON_AddNumberToObject(key_obj, "protocol", node->key.protocol);
    cJSON_AddNumberToObject(key_obj, "ip_type", node->key.ip_type);

    cJSON_AddItemToObject(flow_obj, "key", key_obj);

    curr_session = node->first_session;
    while (curr_session)
    {
        cJSON *sess_obj = cJSON_CreateObject();
        cJSON *devices_array = cJSON_CreateArray();

        cJSON_AddNumberToObject(sess_obj, "session_idx", session_idx);

        cJSON_AddNumberToObject(sess_obj, "start_state", curr_session->start_state);
        cJSON_AddNumberToObject(sess_obj, "end_state", curr_session->end_state);

        if (curr_session->messages.head) {
            cJSON_AddNumberToObject(sess_obj, "start_time", get_precise_time(&curr_session->messages.head->timestamp));
        } else {
            cJSON_AddNumberToObject(sess_obj, "start_time", 0);
        }

        if (curr_session->messages.tail) {
            cJSON_AddNumberToObject(sess_obj, "end_time", get_precise_time(&curr_session->messages.tail->timestamp));
        } else {
            cJSON_AddNumberToObject(sess_obj, "end_time", 0);
        }

        for (device_idx = 0; device_idx < DEVICES_IN_FLOW; device_idx++)
        {
            cJSON *dev_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(dev_obj, "packets_sent", curr_session->devices[device_idx].data.packets_sent);
            cJSON_AddNumberToObject(dev_obj, "bytes_sent", curr_session->devices[device_idx].data.bytes_sent);

            cJSON_AddItemToArray(devices_array, dev_obj);
        }

        cJSON_AddItemToObject(sess_obj, "devices", devices_array);
        cJSON_AddItemToArray(sessions_array, sess_obj);

        curr_session = curr_session->next;
        session_idx++;
    }

    cJSON_AddItemToObject(flow_obj, "sessions", sessions_array);

    cJSON_AddItemToArray(flows_array, flow_obj);
}

static void add_ip_to_json_callback(ip_tree_node_t *node, ip_version_e ip_addr_type, void *context)
{
    if (!node || !context) return;

    cJSON *ip_arr = (cJSON *)context;
    char ip_buf[INET6_ADDRSTRLEN] = {0};

    if (node->ip == NULL) {
        printf("[DEBUG] Found a node with NULL IP! Skipping...\n");
        return;
    }

    get_ip_str((const ip_addr_t *)node->ip, ip_addr_type, ip_buf, sizeof(ip_buf));

    cJSON *ip_obj = cJSON_CreateObject();
    if (!ip_obj) return;

    cJSON_AddStringToObject(ip_obj, "ip", ip_buf);
    cJSON_AddNumberToObject(ip_obj, "packets", (double)node->stats.total_packets);

    cJSON_AddNumberToObject(ip_obj, "bytes", (double)node->stats.total_bytes);

    if (!cJSON_AddItemToArray(ip_arr, ip_obj)) {
        cJSON_Delete(ip_obj);
    }
}

cJSON* wrap_ip_tree(ip_tree_t *tree)
{
    cJSON *ips_array = cJSON_CreateArray();

    if (tree && tree->root)
    {
        ip_tree_iterate(tree, add_ip_to_json_callback, ips_array);
    }

    return ips_array;
}

static void get_mac_str(const uint8_t *mac, char *buf)
{
    snprintf(buf, MAC_ADDR_SIZE, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void add_mac_to_json_callback(mac_node_t *node, void *context)
{
    cJSON *mac_array = (cJSON *)context;
    cJSON *mac_obj = cJSON_CreateObject();
    cJSON *ipv4_data = cJSON_CreateObject();
    cJSON *ipv6_data = cJSON_CreateObject();

    char mac_buf[18];

    get_mac_str(node->mac_addr, mac_buf);

    cJSON_AddStringToObject(mac_obj, "mac", mac_buf);
    cJSON_AddNumberToObject(mac_obj, "packets", (double)node->packet_count);
    cJSON_AddNumberToObject(mac_obj, "bytes", (double)node->total_bytes);

    if (node->ipv4_tree)
    {
        cJSON_AddItemToObject(ipv4_data, "history", wrap_ip_tree(node->ipv4_tree));
        cJSON_AddNumberToObject(ipv4_data, "count", node->ipv4_tree->total_unique_ips);

        cJSON_AddNumberToObject(ipv4_data, "packets", (double)node->ipv4_tree->root->stats.total_packets);
        cJSON_AddNumberToObject(ipv4_data, "bytes", (double)node->ipv4_tree->root->stats.total_bytes);

        cJSON_AddItemToObject(mac_obj, "ipv4_data", ipv4_data);
    }
    if (node->ipv6_tree)
    {
        cJSON_AddItemToObject(ipv6_data, "history", wrap_ip_tree(node->ipv6_tree));
        cJSON_AddNumberToObject(ipv6_data, "count", node->ipv6_tree->total_unique_ips);

        cJSON_AddNumberToObject(ipv6_data, "packets", (double)node->ipv6_tree->root->stats.total_packets);
        cJSON_AddNumberToObject(ipv6_data, "bytes", (double)node->ipv6_tree->root->stats.total_bytes);

        cJSON_AddItemToObject(mac_obj, "ipv6_data", ipv6_data);
    }

    cJSON_AddItemToArray(mac_array, mac_obj);
}

cJSON* wrap_mac_table(mac_table_t *table)
{
    cJSON *mac_array = cJSON_CreateArray();
    if (table)
    {
        mac_table_iterate(table, add_mac_to_json_callback, mac_array);
    }
    return mac_array;
}

cJSON* wrap_parser_results(file_analysis_context_t *core)
{
    cJSON *root = cJSON_CreateObject();

    if (!core)
    {
        cJSON_AddStringToObject(root, "status", "error_null_core");
    }
    else
    {
        // L2
        if(core->mac_table)
        {
            cJSON_AddItemToObject(root, "mac_stats", wrap_mac_table(core->mac_table));
        }

        // L3
        if(core->ipv4_tree)
        {
            cJSON_AddItemToObject(root, "global_ipv4_stats", wrap_ip_tree(core->ipv4_tree));
        }
        if(core->ipv6_tree)
        {
            cJSON_AddItemToObject(root, "global_ipv6_stats", wrap_ip_tree(core->ipv6_tree));
        }

        // L4
        cJSON *flows_array = cJSON_CreateArray();
        if (core->flow_table)
        {
            flow_table_iterate(core->flow_table, add_flow_to_json_callback, flows_array);
        }
        cJSON_AddItemToObject(root, "flows", flows_array);

        // General stats
        cJSON_AddNumberToObject(root, "total_packets", core->total_packets);
        cJSON_AddNumberToObject(root, "total_unique_macs", core->mac_table ? core->mac_table->total_devices : 0);
        cJSON_AddStringToObject(root, "status", "analysis_complete");
    }

    return root;
}