#include "flow_table.h"
#include "xxhash.h"
#include "l7_handler.h"

/**
 * @brief Computes the hash table bucket index for a flow key.
 *
 * @param flow_key Pointer to the flow key to hash.
 * @return Bucket index in range [0, FLOW_HASH_SIZE).
 */
static uint32_t calculate_hash(const flow_key_t *flow_key)
{
    return (uint32_t)(XXH3_64bits(flow_key, FLOW_KEY_SIZE) & (FLOW_HASH_SIZE - 1));
}

/**
 * @brief Allocates and initialises an empty flow table.
 *
 * @return Pointer to the new table, or NULL on allocation failure.
 */
flow_table_t *flow_table_init()
{
    flow_table_t *table = (flow_table_t *)malloc(FLOW_TABLE_SIZE);

    if (table)
    {
        memset(table->buckets, 0, sizeof(table->buckets));
        table->flow_count = 0;
    }

    return table;
}

/**
 * @brief Converts a binary IP address to a human-readable string.
 *
 * @param ip_addr  Pointer to the binary IP address.
 * @param ip_ver   IP version (IPv4 or IPv6).
 * @param out_buf  Caller-supplied output buffer.
 * @param buf_len  Size of out_buf in bytes.
 * @return out_buf (always valid, contains "Unknown" on failure).
 */
char *get_ip_str(const ip_addr_t *ip_addr, ip_version_e ip_ver, char *out_buf, size_t buf_len)
{
    int addr_family = (ip_ver == IP_VERSION_6) ? (AF_INET6) : (AF_INET);

    if (inet_ntop(addr_family, ip_addr, out_buf, buf_len) == NULL)
    {
        snprintf(out_buf, buf_len, "Unknown");
    }

    return out_buf;
}

/**
 * @brief Returns a human-readable string describing the TCP/UDP/ICMP session state.
 *
 * @param session  Pointer to the session node.
 * @param protocol IP protocol number (IPPROTO_TCP, IPPROTO_UDP, etc.).
 * @return A constant string describing the current state.
 */
const char *get_tcp_state_str(session_node_t *session, uint8_t protocol)
{
    const char *state_str = "ACTIVE";

    if (protocol == IPPROTO_UDP)
    {
        state_str = "UDP_FLOW";
    }
    else if (protocol == IPPROTO_ICMP)
    {
        state_str = "ICMP_ECHO";
    }
    else if (protocol != IPPROTO_TCP)
    {
        state_str = "N/A";
    }
    else if (session->end_state == FLOW_TABLE_TCP_END_STATE_CLOSED_GRACEFULLY)
    {
        state_str = "CLOSED (OK)";
    }
    else if (session->end_state == FLOW_TABLE_TCP_END_STATE_CLOSED_UNGRACEFULLY)
    {
        state_str = "RESET/TIMEOUT";
    }
    else if (session->start_state == FLOW_TABLE_TCP_START_STATE_HANDSHAKE_COMPLETE)
    {
        state_str = "ESTABLISHED";
    }

    return state_str;
}

/**
 * @brief Prints a human-readable summary of all flows in the table to stdout.
 *
 * @param table Pointer to the flow table.
 */
void flow_table_print_report(flow_table_t *table)
{
    if (table)
    {
        printf("\n%-40s %-6s <-> %-40s %-6s | Pro | Sess | Pkts | Bytes | State\n",
               "Src IP", "Port", "Dst IP", "Port");
        printf("------------------------------------------------------------------------------------------------------------------\n");

        for (int bucket_idx = 0; bucket_idx < FLOW_HASH_SIZE; bucket_idx++)
        {
            flow_node_t *flow_node = table->buckets[bucket_idx];

            while (flow_node)
            {
                session_node_t *session = flow_node->first_session;
                int             session_idx = 1;
                char            src_ip_str[INET6_ADDRSTRLEN];
                char            dst_ip_str[INET6_ADDRSTRLEN];
                uint32_t        total_pkts;
                uint32_t        total_bytes;
                const char      *state_str;

                get_ip_str(&flow_node->key.src_ip, flow_node->key.ip_type, src_ip_str, sizeof(src_ip_str));
                get_ip_str(&flow_node->key.dst_ip, flow_node->key.ip_type, dst_ip_str, sizeof(dst_ip_str));

                while (session)
                {
                    total_pkts  = session->devices[0].data.packets_sent + session->devices[1].data.packets_sent;
                    total_bytes = session->devices[0].data.bytes_sent   + session->devices[1].data.bytes_sent;
                    state_str   = get_tcp_state_str(session, flow_node->key.protocol);

                    printf("%-40s %-6u <-> %-40s %-6u | %-3u | #%-3d | %-4u | %-5u | %s\n",
                           src_ip_str, flow_node->key.src_port,
                           dst_ip_str, flow_node->key.dst_port,
                           flow_node->key.protocol, session_idx++,
                           total_pkts, total_bytes, state_str);

                    session = session->next;
                }

                flow_node = flow_node->next;
            }
        }
    }
}

/**
 * @brief Frees all memory owned by the flow table, including all nodes and sessions.
 *
 * @param table Pointer to the flow table to free.
 */
void flow_table_free_table(flow_table_t *table)
{
    flow_node_t *current_node;
    flow_node_t *next_node;

    if (table)
    {
        for (int bucket_idx = 0; bucket_idx < FLOW_HASH_SIZE; bucket_idx++)
        {
            current_node = table->buckets[bucket_idx];

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

/**
 * @brief Frees a linked list of message nodes.
 *
 * @param msg_head Head of the message linked list to free.
 */
void flow_table_free_messages(message_node_t *msg_head)
{
    message_node_t *next_msg;

    while (msg_head)
    {
        next_msg = msg_head->next;
        free(msg_head);
        msg_head = next_msg;
    }
}

/**
 * @brief Frees a session node and all its associated resources.
 *
 * @param session Pointer to the session node to free.
 */
void flow_table_free_session(session_node_t *session)
{
    if (session)
    {
        flow_table_free_messages(session->messages.head);
        flow_table_free_messages(session->devices[0].ooo_buffer);
        flow_table_free_messages(session->devices[1].ooo_buffer);

        l7_session_state_free(session->l7);

        // Free the session node
        free(session);
    }
}

/**
 * @brief Frees a flow node and all its sessions.
 *
 * @param flow_node Pointer to the flow node to free.
 */
void flow_table_free_node(flow_node_t *flow_node)
{
    session_node_t *current_session;
    session_node_t *next_session;

    if (flow_node)
    {
        // Free messages linked list
        current_session = flow_node->first_session;

        while (current_session)
        {
            next_session = current_session->next;
            flow_table_free_session(current_session);
            current_session = next_session;
        }

        // Free the flow node
        free(flow_node);
    }
}

/**
 * @brief Builds a canonical, direction-independent flow key from a packet.
 *
 * The key is normalised so that the lower IP address is always stored as src,
 * ensuring that packets from both directions map to the same key. The
 * first_dev output indicates which direction was considered "source".
 *
 * @param pkt_info  Parsed packet information.
 * @param first_dev Output: which endpoint is treated as the flow initiator.
 * @return Populated flow_key_t.
 */
static flow_key_t create_flow_key(packet_info_t *pkt_info, flow_table_first_device_e *first_dev)
{
    flow_key_t key;
    ip_addr_t  ip_src = pkt_info->ip_info.src_ip;
    ip_addr_t  ip_dst = pkt_info->ip_info.dst_ip;
    int        addr_cmp;

    memset(&key, 0, FLOW_KEY_SIZE);

    if (pkt_info->ip_info.ip_version == IP_VERSION_4)
    {
        key.ip_type = IP_VERSION_4;

        //check if src addr truely is smaller than dest addres
        addr_cmp = memcmp(&ip_src, &ip_dst, IPV4_BYTES);
    }
    else
    {
        key.ip_type = IP_VERSION_6;

        //check if src addr truely is smaller than dest addres
        addr_cmp = memcmp(&ip_src, &ip_dst, IPV6_BYTES);
    }

    if (addr_cmp <= 0)
    {
        key.src_ip   = ip_src;
        key.dst_ip   = ip_dst;
        key.src_port = pkt_info->port_info.src_port;
        key.dst_port = pkt_info->port_info.dst_port;
        *first_dev   = FLOW_TABLE_FIRST_DEVICE_SRC;
    }
    else
    {
        key.src_ip   = ip_dst;
        key.dst_ip   = ip_src;
        key.src_port = pkt_info->port_info.dst_port;
        key.dst_port = pkt_info->port_info.src_port;
        *first_dev   = FLOW_TABLE_FIRST_DEVICE_DST;
    }

    key.protocol = pkt_info->ip_info.ip_proto;

    return key;
}

/**
 * @brief Looks up or creates the flow node for an incoming packet.
 *
 * Hashes the packet into a canonical flow key, searches the bucket chain for
 * a matching node, and allocates a new node when none is found.
 *
 * @param table    The flow hash table.
 * @param pkt_info Parsed packet information.
 * @return A struct containing the return code, pointer to the matched/new node,
 *         and the device index for the packet's sender.
 */
flow_table_process_packet_ret_t flow_table_process_packet(flow_table_t *table, packet_info_t *pkt_info)
{
    flow_table_process_packet_ret_t    ret_struct = {0};
    flow_table_process_packet_return_e ret_code   = FLOW_TABLE_PROCESS_PACKET_SUCCESS;
    flow_table_first_device_e          first_dev  = FLOW_TABLE_FIRST_DEVICE_SRC;
    flow_key_t                         key;
    uint32_t                           hash;
    flow_node_t                        *node;

    if (table && pkt_info)
    {
        key  = create_flow_key(pkt_info, &first_dev);
        hash = calculate_hash(&key);
        node = table->buckets[hash];

        while (node && (memcmp(&node->key, &key, FLOW_KEY_SIZE) != 0))
        {
            node = node->next;
        }

        // If node not found create a new one
        if (!node)
        {
            node = (flow_node_t *)calloc(1, FLOW_NODE_SIZE);

            if (!node)
            {
                ret_code = FLOW_TABLE_PROCESS_PACKET_MALLOC_FLOW_NODE_ERROR;
            }
            else
            {
                node->key           = key;
                node->first_session = NULL;
                node->last_session  = NULL;

                // Adding the new node as at the head of the bucket
                node->next           = table->buckets[hash];
                table->buckets[hash] = node;
                table->flow_count++;

                ret_code = FLOW_TABLE_PROCESS_PACKET_ADDED_NEW_NODE_SUCCESS;
            }
        }
        else
        {
            ret_code = FLOW_TABLE_PROCESS_PACKET_ADD_TO_EXISTING_FLOW_SUCCESS;
        }
    }

    ret_struct.ret_code      = ret_code;
    ret_struct.flow_node_ptr = node;
    ret_struct.dev_idx       = first_dev;

    return ret_struct;
}

/**
 * @brief Appends a packet to the appropriate session within a flow node.
 *
 * Creates a new session when none exists or when the last session has timed out.
 * Updates per-device packet and byte counters on success.
 *
 * @param flow_node The flow node that owns this session chain.
 * @param pkt_info  Parsed packet information.
 * @param src_dev   Which device index (0 or 1) sent this packet.
 * @return Status code indicating success or the type of failure.
 */
flow_table_process_packet_return_e flow_table_insert_to_session(flow_node_t *flow_node,
                                                                 packet_info_t *pkt_info,
                                                                 flow_table_first_device_e src_dev)
{
    flow_table_process_packet_return_e ret_code       = FLOW_TABLE_PROCESS_PACKET_SUCCESS;
    message_node_t                     *new_msg;
    session_node_t                     *current_session;
    session_node_t                     *new_session;
    struct timeval                     time_diff;

    // Create and add new message to session
    new_msg = (message_node_t *)calloc(1, MESSAGE_NODE_SIZE);

    if (!new_msg)
    {
        printf("error: failed to malloc non tcp message in flow_table_insert_to_session!");
        ret_code = FLOW_TABLE_PROCESS_PACKET_MALLOC_MESSAGE_NODE_ERROR;
    }
    else
    {
        new_msg->timestamp            = pkt_info->cap_info.ts;
        new_msg->payload_len          = pkt_info->offsets.payload_len;
        new_msg->total_packet_len     = pkt_info->cap_info.wire_len;
        new_msg->packet_start_pointer = pkt_info->offsets.packet_start_pointer;
        new_msg->tcp_flags            = pkt_info->port_info.tcp_flags;
        new_msg->dev_idx              = (uint8_t)src_dev;
        new_msg->next                 = NULL;

        current_session = flow_node->last_session;

        if (current_session)
        {
            timersub(&pkt_info->cap_info.ts, &current_session->timestamp, &time_diff);
        }

        if (!current_session || (time_diff.tv_sec >= FLOW_TABLE_TIMEOUT))
        {
            new_session = (session_node_t *)calloc(1, SESSION_NODE_SIZE);

            if (!new_session)
            {
                ret_code = FLOW_TABLE_PROCESS_PACKET_MALLOC_SESSION_ERROR;
                printf("error: failed to malloc non tcp session in flow_table_insert_to_session!");
                free(new_msg);
            }
            else
            {
                ret_code = FLOW_TABLE_PROCESS_PACKET_ADDED_NEW_SESSION_SUCCESS;

                new_session->messages.head = new_msg;
                new_session->messages.tail = new_msg;

                if (!flow_node->first_session)
                {
                    flow_node->last_session  = new_session;
                    flow_node->first_session = new_session;
                }
                else
                {
                    flow_node->last_session->next = new_session;
                    flow_node->last_session       = new_session;
                }

                current_session = new_session;
            }
        }
        else
        {
            ret_code = FLOW_TABLE_PROCESS_PACKET_ADD_TO_EXISTING_FLOW_SUCCESS;

            current_session->messages.tail->next = new_msg;
            current_session->messages.tail       = current_session->messages.tail->next;
        }

        if (ret_code >= FLOW_TABLE_PROCESS_PACKET_SUCCESS)
        {
            current_session->timestamp = pkt_info->cap_info.ts;

            // Update device data
            current_session->devices[src_dev].data.packets_sent++;
            current_session->devices[src_dev].data.bytes_sent += pkt_info->cap_info.wire_len;
        }
    }

    return ret_code;
}

/**
 * @brief Iterates over every flow node in the table and invokes a callback.
 *
 * @param table    The flow table to iterate.
 * @param callback Function called for each flow node.
 * @param context  Caller-supplied context pointer forwarded to the callback.
 */
void flow_table_iterate(flow_table_t *table, flow_callback_fn callback, void *context)
{
    flow_node_t *curr_node;
    flow_node_t *next_node;

    if (table && callback)
    {
        for (int bucket_idx = 0; bucket_idx < FLOW_HASH_SIZE; bucket_idx++)
        {
            curr_node = table->buckets[bucket_idx];

            while (curr_node)
            {
                next_node = curr_node->next;
                callback(curr_node, context);
                curr_node = next_node;
            }
        }
    }
}
