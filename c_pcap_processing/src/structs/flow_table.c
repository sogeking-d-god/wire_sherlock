#include "flow_table.h"

static uint32_t calculate_hash(flow_key_t *key)
{
    uint32_t hash = FLOW_HASH_CONST;

    hash ^= key->src_ip;
    hash ^= key->dst_ip;
    hash ^= key->src_port;
    hash ^= key->dst_port;
    hash ^= key->protocol;
    return hash % HASH_SIZE;
}

flow_table_t* flow_table_init()
{
    flow_table_t *table = (flow_table_t*)malloc(sizeof(flow_table_t));
    if (table)
    {
        memset(table->buckets, 0, sizeof(table->buckets));
        table->flow_count = 0;
    }
    return table;
}

void flow_table_print_report(flow_table_t *table)
{
    if (!table) return;

    printf("\n--- Flow Table Report ---\n");
    printf("Total Flows Detected: %u\n", table->flow_count);
    printf("------------------------------------------------------------------\n");
    printf("%-15s %-6s <-> %-15s %-6s | Pro | Pkts | Bytes\n", "Src IP", "Port", "Dst IP", "Port");
    printf("------------------------------------------------------------------\n");

    for (int i = 0; i < HASH_SIZE; i++) {
        flow_node_t *node = table->buckets[i];
        while (node) {
            struct in_addr sa, da;
            sa.s_addr = node->devices[0].ip;
            da.s_addr = node->devices[1].ip;

            uint32_t total_pkts = node->devices[0].data.packets_sent + node->devices[1].data.packets_sent;
            uint32_t total_bytes = node->devices[0].data.bytes_sent + node->devices[1].data.bytes_sent;

            // שימוש בבאפרים נפרדים וגדולים מספיק
            char s_str[INET_ADDRSTRLEN];
            char d_str[INET_ADDRSTRLEN];

            // המרה בטוחה למחרוזות
            inet_ntop(AF_INET, &sa, s_str, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &da, d_str, INET_ADDRSTRLEN);

            printf("%-15s %-6u <-> %-15s %-6u | %-3u | %-4u | %-10u\n",
                s_str, node->devices[0].port,
                d_str, node->devices[1].port,
                node->protocol, total_pkts, total_bytes);

            node = node->next;
        }
    }
    printf("------------------------------------------------------------------\n");
}

void flow_table_free_table(flow_table_t *table)
{
    flow_node_t *current_node;
    flow_node_t *next_node;
    if (table)
    {
        for (int i = 0; i < HASH_SIZE; i++)
        {
            current_node = table->buckets[i];
            while (current_node)
            {
                next_node = current_node->next;
                flow_table_free_node(current_node);
                current_node = next_node;
            }
        }

        free(table);
    }
}

void flow_table_free_node(flow_node_t *node)
{
    if (node)
    {
        // Free messages linked list
        message_node_t *current_msg = node->messages.head;
        message_node_t *next_msg;
        while (current_msg)
        {
            next_msg = current_msg->next;
            free(current_msg);
            current_msg = next_msg;
        }

        // Free the flow node
        free(node);
    }
}

static flow_key_t create_flow_key(packet_info_t *info, flow_table_first_device_e * first_dev)
{
    flow_key_t key;
    uint32_t s_addr = *(uint32_t*)info->src_ip.v4;
    uint32_t d_addr = *(uint32_t*)info->dst_ip.v4;

    memset(&key, 0, sizeof(flow_key_t));


    if(info->ip_version == IP_VERSION_6)
    {
        //TODO: add handler for ipv6 later
    }

    else if (s_addr <= d_addr)
    {
        key.src_ip = s_addr;
        key.dst_ip = d_addr;
        key.src_port = info->src_port;
        key.dst_port = info->dst_port;

        *first_dev = FLOW_TABLE_FIRST_DEVICE_SRC;
    }
    else
    {
        key.src_ip = d_addr;
        key.dst_ip = s_addr;
        key.src_port = info->dst_port;
        key.dst_port = info->src_port;

        *first_dev = FLOW_TABLE_FIRST_DEVICE_DST;
    }
    key.protocol = info->ip_proto;

    return key;
}

flow_table_process_packet_return_e flow_table_process_packet(flow_table_t *table, packet_info_t *info)
{
    flow_table_process_packet_return_e ret_val = FLOW_TABLE_PROCESS_PACKET_SUCCESS;

    flow_key_t key;
    uint32_t hash;
    flow_node_t *node;
    flow_table_first_device_e src_dev = FLOW_TABLE_FIRST_DEVICE_SRC;
    message_node_t *msg;

    if (table && info)
    {
        key = create_flow_key(info, &src_dev);
        hash = calculate_hash(&key);
        node = table->buckets[hash];

        while (node && memcmp(&node->key, &key, KEY_SIZE) != 0)
        {
            node = node->next;
        }

        // If node note found create a new one
        if (!node)
        {
            node = (flow_node_t*)calloc(1, FLOW_NODE_SIZE);
            if (!node)
            {
                ret_val = FLOW_TABLE_PROCESS_PACKET_MALLOC_FLOW_NODE_ERROR;
            }
            else
            {
                node->key = key;
                node->protocol = key.protocol;

                // The key src ip is smaller than dst ip
                node->devices[0].ip = key.src_ip;
                node->devices[0].port = key.src_port;
                node->devices[1].ip = key.dst_ip;
                node->devices[1].port = key.dst_port;

                // Adding the new node as at the head of the bucket
                node->next = table->buckets[hash];
                table->buckets[hash] = node;
                table->flow_count++;

            }

            ret_val = FLOW_TABLE_PROCESS_PACKET_ADDED_NEW_NODE_SUCCESS;

        }

        else
        {
            ret_val = FLOW_TABLE_PROCESS_PACKET_ADD_TO_EXISTING_FLOW_SUCCESS;
        }

        // Update device data
        node->devices[src_dev].data.packets_sent++;
        node->devices[src_dev].data.bytes_sent += info->packet_len;

        // Create and add new message node
        msg = (message_node_t*)calloc(1, MESSAGE_NODE_SIZE);
        if (!msg)
        {
            ret_val = FLOW_TABLE_PROCESS_PACKET_MALLOC_MESSAGE_NODE_ERROR;
        }

        else
        {
            //TO DO: fill message node data

            msg->payload_len = info->packet_len;

            msg->next = NULL;

            if (!node->messages.head)
            {
                node->messages.head = msg;
                node->messages.tail = msg;
            }
            else
            {
                node->messages.tail->next = msg;
                node->messages.tail = msg;
            }

        }

    }
    return ret_val;

}