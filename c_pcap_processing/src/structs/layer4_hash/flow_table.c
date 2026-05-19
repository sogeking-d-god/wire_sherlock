#include "flow_table.h"
#include "xxhash.h"

static uint32_t calculate_hash(const flow_key_t *key)
{
    return (uint32_t)(XXH3_64bits(key, FLOW_KEY_SIZE) & (FLOW_HASH_SIZE - 1));
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

const char* get_tcp_state_str(session_node_t *sess, uint8_t protocol)
{
    if (protocol == IPPROTO_UDP) return "UDP_FLOW";
    if (protocol == IPPROTO_ICMP) return "ICMP_ECHO";
    if (protocol != IPPROTO_TCP) return "N/A";

    if (sess->end_state == FLOW_TABLE_TCP_END_STATE_CLOSED_GRACEFULLY) return "CLOSED (OK)";
    if (sess->end_state == FLOW_TABLE_TCP_END_STATE_CLOSED_UNGRACEFULLY) return "RESET/TIMEOUT";
    if (sess->start_state == FLOW_TABLE_TCP_START_STATE_HANDSHAKE_COMPLETE) return "ESTABLISHED";

    return "ACTIVE";
}

void flow_table_print_report(flow_table_t *table)
{
    if (!table) return;

    printf("\n%-40s %-6s <-> %-40s %-6s | Pro | Sess | Pkts | Bytes | State\n", "Src IP", "Port", "Dst IP", "Port");
    printf("------------------------------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < FLOW_HASH_SIZE; i++)
    {
        flow_node_t *node = table->buckets[i];
        while (node)
        {
            session_node_t *sess = node->first_session;
            int sess_idx = 1;
            char s_str[INET6_ADDRSTRLEN], d_str[INET6_ADDRSTRLEN];

            get_ip_str(&node->key.src_ip, node->key.ip_type, s_str, sizeof(s_str));
            get_ip_str(&node->key.dst_ip, node->key.ip_type, d_str, sizeof(d_str));

            while (sess)
            {
                uint32_t pkts = sess->devices[0].data.packets_sent + sess->devices[1].data.packets_sent;
                uint32_t bytes = sess->devices[0].data.bytes_sent + sess->devices[1].data.bytes_sent;

                const char* state_str = get_tcp_state_str(sess, node->key.protocol);

                printf("%-40s %-6u <-> %-40s %-6u | %-3u | #%-3d | %-4u | %-5u | %s\n",
                    s_str, node->key.src_port, d_str, node->key.dst_port,
                    node->key.protocol, sess_idx++, pkts, bytes,
                    state_str);
                sess = sess->next;
            }
            node = node->next;
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

void flow_table_free_messages(message_node_t *msg)
{
    message_node_t *next_msg;
    while (msg)
    {
        next_msg = msg->next;
        free(msg);
        msg = next_msg;
    }
}

void flow_table_free_session(session_node_t *session)
{
    if (session)
    {
        flow_table_free_messages(session->messages.head);
        flow_table_free_messages(session->devices[0].ooo_buffer);
        flow_table_free_messages(session->devices[1].ooo_buffer);

        // Free the session node
        free(session);
    }
}

void flow_table_free_node(flow_node_t *node)
{
    if (node)
    {
        // Free messages linked list
        session_node_t *current_session = node->first_session;
        session_node_t *next_session;
        while (current_session)
        {
            next_session = current_session->next;
            flow_table_free_session(current_session);
            current_session = next_session;
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
    int cmp;

    memset(&key, 0, FLOW_KEY_SIZE);


    if(info->ip_info.ip_version == IP_VERSION_4)
    {
        key.ip_type = IP_VERSION_4;

        //check if src addr truely is smaller than dest addres
        cmp = memcmp(&s_addr, &d_addr, IPV4_BYTES);
    }
    else
    {
        key.ip_type = IP_VERSION_6;

        //check if src addr truely is smaller than dest addres
        cmp = memcmp(&s_addr, &d_addr, IPV6_BYTES);
    }


    if (cmp <= 0)
    {
        key.src_ip = s_addr;
        key.dst_ip = d_addr;
        key.src_port = info->port_info.src_port;
        key.dst_port = info->port_info.dst_port;

        *first_dev = FLOW_TABLE_FIRST_DEVICE_SRC;
    }
    else
    {
        key.src_ip = d_addr;
        key.dst_ip = s_addr;
        key.src_port = info->port_info.dst_port;
        key.dst_port = info->port_info.src_port;

        *first_dev = FLOW_TABLE_FIRST_DEVICE_DST;
    }
    key.protocol = info->ip_info.ip_proto;

    return key;
}

flow_table_process_packet_ret_t flow_table_process_packet(flow_table_t *table, packet_info_t *info)
{
    flow_table_process_packet_return_e ret_code = FLOW_TABLE_PROCESS_PACKET_SUCCESS;
    flow_table_process_packet_ret_t ret_struct = {0};

    flow_key_t key;
    uint32_t hash;
    flow_node_t *node;
    flow_table_first_device_e src_dev = FLOW_TABLE_FIRST_DEVICE_SRC;
    boolean_e continue_loop = TRUE;

    if (table && info)
    {
        key = create_flow_key(info, &src_dev);
        hash = calculate_hash(&key);
        node = table->buckets[hash];

        while (node && continue_loop)
        {
            if (memcmp(&node->key, &key, FLOW_KEY_SIZE) == 0)
            {
                continue_loop = FALSE;
            }
            else
            {
                node = node->next;
            }
        }

        // If node not found create a new one
        if (!node)
        {
            node = (flow_node_t*)calloc(1, FLOW_NODE_SIZE);
            if (!node)
            {
                ret_code = FLOW_TABLE_PROCESS_PACKET_MALLOC_FLOW_NODE_ERROR;
            }
            else
            {
                node->key = key;
                node->next = NULL;

                node->first_session = NULL;
                node->last_session = NULL;

                // Adding the new node as at the head of the bucket
                node->next = table->buckets[hash];
                table->buckets[hash] = node;
                table->flow_count++;

            }

            ret_code = FLOW_TABLE_PROCESS_PACKET_ADDED_NEW_NODE_SUCCESS;
        }
        else
        {
            ret_code = FLOW_TABLE_PROCESS_PACKET_ADD_TO_EXISTING_FLOW_SUCCESS;
        }
    }
    ret_struct.ret_code = ret_code;
    ret_struct.flow_node_ptr = node;
    ret_struct.dev_idx = src_dev;

    return ret_struct;
}

flow_table_process_packet_return_e flow_table_insert_to_session(flow_node_t * flow_node, packet_info_t * info, flow_table_first_device_e src_dev)
{
    flow_table_process_packet_return_e ret_code = FLOW_TABLE_PROCESS_PACKET_SUCCESS;

    message_node_t * msg;
    session_node_t * session_node;
    struct timeval diff;

    // Create and add new message to session
    msg = (message_node_t*)calloc(1, MESSAGE_NODE_SIZE);
    if (!msg)
    {
        printf("error: failed to malloc non tcp message in flow_table_insert_to_session!");
        ret_code = FLOW_TABLE_PROCESS_PACKET_MALLOC_MESSAGE_NODE_ERROR;
    }

    else
    {

        msg->timestamp = info->cap_info.ts;
        msg->payload_len = info->offsets.payload_len;
        msg->total_packet_len = info->cap_info.wire_len;
        msg->packet_start_pointer = info->offsets.packet_start_pointer;

        msg->tcp_flags = info->port_info.tcp_flags;
        msg->next = NULL;

        session_node = flow_node->last_session;
        if(session_node)
        {
            timersub(&info->cap_info.ts, &session_node->timestamp, &diff);
        }
        if (!session_node || (diff.tv_sec >= FLOW_TABLE_TIMEOUT))
        {
            session_node = (session_node_t *)calloc(1,SESSION_NODE_SIZE);
            if(!session_node)
            {
                ret_code = FLOW_TABLE_PROCESS_PACKET_MALLOC_SESSION_ERROR;
                printf("error: failed to malloc non tcp session in flow_table_insert_to_session!");
                free(msg);
            }
            else
            {
                ret_code = FLOW_TABLE_PROCESS_PACKET_ADDED_NEW_SESSION_SUCCESS;


                session_node->messages.head = msg;
                session_node->messages.tail = msg;


                if(!flow_node->first_session)
                {
                    flow_node->last_session = session_node;
                    flow_node->first_session = session_node;
                }
                else
                {
                    flow_node->last_session->next = session_node;
                    flow_node->last_session = session_node;
                }
            }
        }
        else
        {
            ret_code = FLOW_TABLE_PROCESS_PACKET_ADD_TO_EXISTING_FLOW_SUCCESS;

            session_node->messages.tail->next = msg;
            session_node->messages.tail = session_node->messages.tail->next;
        }

        if(ret_code >= FLOW_TABLE_PROCESS_PACKET_SUCCESS)
        {
            session_node->timestamp = info->cap_info.ts;

            // Update device data
            session_node->devices[src_dev].data.packets_sent++;
            session_node->devices[src_dev].data.bytes_sent += info->cap_info.wire_len;
        }
    }
    return ret_code;
}

void flow_table_iterate(flow_table_t *table, flow_callback_fn callback, void *context)
{
    flow_node_t *curr, *next;
    if(table && callback)
    {
        for (int i = 0; i < FLOW_HASH_SIZE; i++)
        {
            curr = table->buckets[i];
            while (curr)
            {
                next = curr->next;
                callback(curr, context);
                curr = next;
            }
        }
    }
}