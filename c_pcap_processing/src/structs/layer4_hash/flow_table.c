#include "flow_table.h"

static uint32_t calculate_hash(flow_key_t *key)
{
    uint32_t hash = FLOW_HASH_CONST;
    uint32_t *src_ptr = (uint32_t *)key->src_ip.v6;
    uint32_t *dst_ptr = (uint32_t *)key->dst_ip.v6;

    // because ipv4 has 1 32 bit word, and ipv6 is the length of 4 ipv4, or the amount of bytes in ipv4 ^ 2
    uint8_t seg_count = (key->ip_type == IP_VERSION_6) ? IPV4_BYTES : 1;

    // because the key size is 32 bits, xor with each 32 bits segment of the address
    for (int i = 0; i < seg_count; i++)
    {
        hash ^= src_ptr[i];
        hash ^= dst_ptr[i];;
    }

    hash ^= key->src_port;
    hash ^= key->dst_port;
    hash ^= key->protocol;
    hash ^= key->ip_type;
    return hash % FLOW_HASH_SIZE;
}

flow_table_t* flow_table_init()
{
    flow_table_t *table = (flow_table_t*)malloc(FLOW_TABLE_SIZE);
    if (table)
    {
        memset(table->buckets, 0, sizeof(table->buckets));
        table->flow_count = 0;
    }
    return table;
}

char* get_ip_str(const ip_addr_t *ip, ip_version_e ver, char * buf, size_t buflen)
{
    int family = (ver == IP_VERSION_6)?(AF_INET6):(AF_INET);

    if (inet_ntop(family, ip, buf, buflen) == NULL)
    {
        snprintf(buf, buflen, "Unknown");
    }
    return buf;
}

void flow_table_print_report(flow_table_t *table)
{
    if (table)
    {
        printf("\n--- Flow Table Report ---\n");
        printf("Total Flows Detected: %u\n", table->flow_count);
        printf("------------------------------------------------------------------------------------------------------\n");
        printf("%-40s %-6s <-> %-40s %-6s | Pro | Pkts | Bytes\n", "Src IP", "Port", "Dst IP", "Port");
        printf("------------------------------------------------------------------------------------------------------\n");

        for (int i = 0; i < FLOW_HASH_SIZE; i++)
        {
            flow_node_t *node = table->buckets[i];
            while (node)
            {
                char s_str[INET6_ADDRSTRLEN];
                char d_str[INET6_ADDRSTRLEN];

                get_ip_str(&node->devices[0].ip, node->key.ip_type, s_str, sizeof(s_str));
                get_ip_str(&node->devices[1].ip, node->key.ip_type, d_str, sizeof(d_str));

                uint32_t total_pkts = node->devices[0].data.packets_sent + node->devices[1].data.packets_sent;
                uint32_t total_bytes = node->devices[0].data.bytes_sent + node->devices[1].data.bytes_sent;

                printf("%-40s %-6u <-> %-40s %-6u | %-3u | %-4u | %-10u\n",
                    s_str, node->devices[0].port,
                    d_str, node->devices[1].port,
                    node->protocol, total_pkts, total_bytes);

                node = node->next;
            }
        }
    }

}

void flow_table_free_table(flow_table_t *table)
{
    flow_node_t *current_node;
    flow_node_t *next_node;
    if (table)
    {
        for (int i = 0; i < FLOW_HASH_SIZE; i++)
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
    ip_addr_t s_addr = info->ip_info.src_ip;
    ip_addr_t d_addr = info->ip_info.dst_ip;
    uint8_t addr_size;
    uint8_t i = 0;
    boolean_e is_src_smaller = TRUE;

    memset(&key, 0, FLOW_KEY_SIZE);

    // addr size  -1 so that the variables i and addr_size can be only 8 bits

    if(info->ip_info.ip_proto == IP_VERSION_4)
    {
        addr_size = IPV4_BYTES - 1;
    }
    else
    {
        addr_size = IPV6_BYTES - 1;
    }

    //check if src addr truely is smaller than dest addres
    for (; i <= addr_size; i++)
    {
        if(s_addr.v6[i] > d_addr.v6[i])
        {
            is_src_smaller = FALSE;
        }
    }

    if (is_src_smaller)
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
    key.protocol = info->ip_info.ip_proto;

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

        while (node && memcmp(&node->key, &key, FLOW_KEY_SIZE) != 0)
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