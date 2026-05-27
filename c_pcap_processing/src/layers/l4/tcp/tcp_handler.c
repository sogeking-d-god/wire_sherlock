#include "tcp_handler.h"

// Prototypes for static helper functions
static uint32_t calculate_next_seq(message_node_t *msg);
static void drain_ooo_buffer(session_node_t *session, uint8_t dev_idx);
static void insert_sorted_ooo(session_node_t *session, message_node_t *new_msg, uint8_t dev_idx);

/**
 * @brief Returns the last open session for the flow, or creates a new one if needed.
 * A new session is created when there is no existing session, the last session
 * timed out, or a new SYN arrives on an already-closed session.
 *
 * @param node The flow node that owns the session chain.
 * @param info Parsed packet information for the current packet.
 * @return Pointer to the active session, or NULL on allocation failure.
 */
static session_node_t *get_or_create_tcp_session(flow_node_t *node, packet_info_t *info)
{
    session_node_t *session = node->last_session;
    uint8_t flags = info->port_info.tcp_flags;
    struct timeval diff;
    boolean_e is_timeout = FALSE;
    boolean_e is_new_syn = ((flags & TH_SYN) && !(flags & TH_ACK));
    session_node_t *new_session;

    // if session already exists, check for timeout
    if (session)
    {
        timersub(&info->cap_info.ts, &session->timestamp, &diff);
        if (diff.tv_sec >= FLOW_TABLE_TIMEOUT)
        {
            is_timeout = TRUE;
            if (session->end_state == FLOW_TABLE_TCP_END_STATE_NOT_CLOSED)
            {
                session->end_state = FLOW_TABLE_TCP_END_STATE_CLOSED_UNGRACEFULLY;
            }
        }
    }

    // create new session if no session exists, last session timed out, or new SYN on closed session
    if (!session || is_timeout || (is_new_syn && session->end_state != FLOW_TABLE_TCP_END_STATE_NOT_CLOSED))
    {
        new_session = (session_node_t *)calloc(1, SESSION_NODE_SIZE);
        if (!new_session)
        {
            printf("error: failed to malloc new tcp session!\n");
        }
        else
        {
            new_session->start_state = FLOW_TABLE_TCP_START_STATE_IDLE;
            new_session->end_state = FLOW_TABLE_TCP_END_STATE_NOT_CLOSED;

            if (!node->first_session)
            {
                node->first_session = new_session;
            }
            else
            {
                node->last_session->next = new_session;
            }
            node->last_session = new_session;
        }

        session = new_session;
    }

    return session;
}

/**
 * @brief Updates the TCP handshake and teardown state machine for a session.
 *
 * @param session The session whose state should be updated.
 * @param info Parsed packet information for the current packet.
 * @param first_dev Device index of the packet sender.
 */
static void update_tcp_state(session_node_t *session, packet_info_t *info, flow_table_first_device_e first_dev)
{
    uint8_t flags = info->port_info.tcp_flags;
    flow_table_tcp_end_state_e current_fin;

    if (session->start_state != FLOW_TABLE_TCP_START_STATE_HANDSHAKE_COMPLETE)
    {
        if ((flags & TH_SYN) && !(flags & TH_ACK))
        {
            session->start_state = FLOW_TABLE_TCP_START_STATE_SYN_SENT;
        }
        else if ((flags & TH_SYN) && (flags & TH_ACK) && (session->start_state == FLOW_TABLE_TCP_START_STATE_SYN_SENT))
        {
            session->start_state = FLOW_TABLE_TCP_START_STATE_SYN_ACK_SENT;
            // responders next_expected_seq has already been updated outside the function, because it was the first message from that device that had data.
            // session->devices[first_dev].data.next_expected_seq = session->devices[first_dev].data.last_seq + 1;
        }
        else if ((flags & TH_ACK) && (session->start_state == FLOW_TABLE_TCP_START_STATE_SYN_ACK_SENT))
        {
            session->start_state = FLOW_TABLE_TCP_START_STATE_HANDSHAKE_COMPLETE;
        }
    }

    if (flags & TH_RST)
    {
        session->end_state = FLOW_TABLE_TCP_END_STATE_CLOSED_UNGRACEFULLY;
    }
    else if (flags & TH_FIN)
    {
        current_fin = (first_dev == FLOW_TABLE_FIRST_DEVICE_SRC) ?
            FLOW_TABLE_TCP_END_STATE_FIN_SENT_BY_SRC : FLOW_TABLE_TCP_END_STATE_FIN_SENT_BY_DEST;

        if (session->end_state == FLOW_TABLE_TCP_END_STATE_NOT_CLOSED)
        {
            session->end_state = current_fin;
        }
        else if (session->end_state != current_fin && session->end_state < FLOW_TABLE_TCP_END_STATE_CLOSED_GRACEFULLY)
        {
            session->end_state = FLOW_TABLE_TCP_END_STATE_CLOSED_GRACEFULLY;
        }
    }
}

/**
 * @brief Trims already-seen bytes from the front of a message node.
 *
 * If the message starts before the expected sequence number (partial overlap),
 * the seq_num, payload_len, and data_ptr are adjusted so only new data remains.
 *
 * @param msg The message node to trim.
 * @param expected The next expected sequence number for this direction.
 */
static void trim_message_node(message_node_t *msg, uint32_t expected)
{
    uint32_t overlap = expected - msg->seq_num;

    if (overlap < (uint32_t)TCP_SEQ_HALF && msg->seq_num != expected)
    {
        msg->seq_num = expected;

        if (msg->payload_len >= overlap)
        {
            msg->payload_len -= overlap;
            msg->data_ptr += overlap;
        }
        else
        {
            // Only SYN/FIN flags impacted the expected seq
            msg->payload_len = 0;
        }
    }
}

/**
 * @brief Flushes out-of-order buffered messages that are now in sequence.
 *
 * After a new in-order message is appended, this function drains the OOO
 * buffer for the given device, attaching any newly-contiguous messages to
 * the main message list and freeing duplicates.
 *
 * @param session The session whose OOO buffer should be drained.
 * @param dev_idx The device index (0 or 1) whose buffer to drain.
 */
static void drain_ooo_buffer(session_node_t *session, uint8_t dev_idx)
{
    uint32_t expected = session->devices[dev_idx].data.next_expected_seq;
    message_node_t *ooo_temp;
    uint32_t expected_after_new_msg;

    while (session->devices[dev_idx].ooo_buffer != NULL && (expected - session->devices[dev_idx].ooo_buffer->seq_num < (uint32_t)TCP_SEQ_HALF))
    {
        // detatch from out of order list
        ooo_temp = session->devices[dev_idx].ooo_buffer;
        session->devices[dev_idx].ooo_buffer = ooo_temp->next;

        expected_after_new_msg = calculate_next_seq(ooo_temp);

        // Attach to main list if msg contains some new info
        if ((expected_after_new_msg - expected) < (uint32_t)TCP_SEQ_HALF && expected_after_new_msg != expected)
        {
            trim_message_node(ooo_temp, expected);
            session->messages.tail->next = ooo_temp;
            session->messages.tail = ooo_temp;
            ooo_temp->next = NULL;
            session->devices[dev_idx].data.next_expected_seq = expected_after_new_msg;
            expected = expected_after_new_msg;
        }
        // the data of the message was already processed
        else
        {
            free(ooo_temp);
        }
    }
}

/**
 * @brief Processes a TCP packet and updates the flow's session and message list.
 *
 * Handles session creation, TCP state tracking, sequence number management,
 * in-order delivery, out-of-order buffering, and retransmission detection.
 *
 * @param node The flow node for this connection.
 * @param info Parsed packet information.
 * @param tcp_data Pointer to the start of the TCP header bytes.
 * @param dev_idx Device index of the sender.
 * @return Status code indicating success or the type of failure.
 */
flow_table_process_packet_return_e tcp_handler_process_flow_update(flow_node_t *node, packet_info_t *info, const uint8_t *tcp_data, flow_table_first_device_e dev_idx)
{
    flow_table_process_packet_return_e ret_val = FLOW_TABLE_PROCESS_PACKET_SUCCESS;
    struct tcphdr *tcp_header;
    uint32_t seq;
    uint32_t ack;
    uint32_t expected;
    uint32_t expected_after_new_msg;
    uint32_t payload_len;
    uint32_t control;
    message_node_t *msg;
    session_node_t *session;

    msg = NULL;

    if (node && info && tcp_data)
    {
        session = get_or_create_tcp_session(node, info);
        if (!session)
        {
            ret_val = FLOW_TABLE_PROCESS_PACKET_MALLOC_SESSION_ERROR;
            printf("error calloc session failed in tcp tcp_handler_process_flow_update");
        }
        else
        {
            // Update session statistics and timestamp
            session->timestamp = info->cap_info.ts;
            session->devices[dev_idx].data.packets_sent++;
            session->devices[dev_idx].data.bytes_sent += info->cap_info.wire_len;

            // Extract TCP info
            tcp_header = (struct tcphdr *)tcp_data;
            seq = ntohl(tcp_header->th_seq);
            ack = ntohl(tcp_header->th_ack);
            payload_len = info->cap_info.wire_len - (info->offsets.l4_offset + (tcp_header->th_off * 4));
            control = (tcp_header->th_flags & (TH_SYN | TH_FIN)) ? 1 : 0;
            expected = session->devices[dev_idx].data.next_expected_seq;
            expected_after_new_msg = seq + payload_len + control;

            // Mid-stream pickup: if this device's seq was never initialized 
            // and this packet actually advances the sequence space => anchor to it.  (to not alert retransmission).
            if (!session->devices[dev_idx].seq_initialized && (payload_len + control) > 0)
            {
                session->devices[dev_idx].data.next_expected_seq = seq;
                expected = seq;
                session->devices[dev_idx].seq_initialized = TRUE;
            }

            // Create new message node
            msg = (message_node_t *)calloc(1, MESSAGE_NODE_SIZE);
            if (msg)
            {
                msg->seq_num = seq;
                msg->payload_len = payload_len;
                msg->timestamp = info->cap_info.ts;
                msg->tcp_flags = info->port_info.tcp_flags;
                msg->data_ptr = info->offsets.payload_offset;
                msg->total_packet_len = info->cap_info.wire_len;
                msg->packet_start_pointer = info->offsets.packet_start_pointer;
                msg->dev_idx = (uint8_t)dev_idx;
                msg->next = NULL;

                // First message in flow
                if (!session->messages.head)
                {
                    session->messages.head = msg;
                    session->messages.tail = msg;
                    // if it doeasnt have data the next expected seq will be updated in the future in the mid stream pickup condition, when a message with data arrives.
                    if ((payload_len + control) > 0)
                    {
                        session->devices[dev_idx].data.next_expected_seq = expected_after_new_msg;
                    }
                }
                // in order message in flow
                else if (expected - seq < (uint32_t)TCP_SEQ_HALF && (expected_after_new_msg - expected) < (uint32_t)TCP_SEQ_HALF && expected_after_new_msg != expected)
                {
                    trim_message_node(msg, expected);

                    session->messages.tail->next = msg;
                    session->messages.tail = msg;

                    session->devices[dev_idx].data.next_expected_seq = expected_after_new_msg;

                    drain_ooo_buffer(session, dev_idx);
                }
                // Future packet - store in out of order sorted buffer
                else if (seq - expected < (uint32_t)TCP_SEQ_HALF)
                {
                    insert_sorted_ooo(session, msg, dev_idx);
                }
                // Retransmission
                else
                {
                    info->is_retransmit = TRUE;
                    session->devices[dev_idx].data.retransmit_count++;
                    free(msg);
                }
            }
            else
            {
                ret_val = FLOW_TABLE_PROCESS_PACKET_MALLOC_MESSAGE_NODE_ERROR;
                printf("error calloc msg failed in tcp tcp_handler_process_flow_update");
            }

            // Update shared state
            session->devices[dev_idx].data.last_seq = seq;
            session->devices[dev_idx].data.last_ack = ack;
            update_tcp_state(session, info, dev_idx);
        }
    }
    else
    {
        ret_val = FLOW_TABLE_PROCESS_PACKET_INVALID_PARAMS_ERROR;
        if (!node)
        {
            printf("error node is NULL in tcp_handler_process_flow_update\n");
        }
        if (!info)
        {
            printf("error info is NULL in tcp_handler_process_flow_update\n");
        }
        if (!tcp_data)
        {
            printf("error tcp_data is NULL in tcp_handler_process_flow_update\n");
        }
    }

    return ret_val;
}

/**
 * @brief Parses the TCP header fields from a raw packet and populates packet_info_t.
 *
 * Validates the captured length, computes the payload offset, and extracts
 * ports and TCP flags. Sets payload_offset to 0 on corrupt or truncated packets.
 *
 * @param data Pointer to the start of the TCP header bytes.
 * @param info Packet info struct to populate.
 * @return PROTO_HANDLER_SUCCESS, PROTO_HANDLER_CORRUPT_PACKET, or PROTO_HANDLER_NOT_RECORDED_PACKET.
 */
proto_handler_return_codes_e handle_tcp_packet(const uint8_t *data, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;
    uint8_t header_len;
    struct tcphdr *tcp_header;

    info->offsets.payload_offset = info->offsets.l4_offset + TCP_HEADER_MIN_LEN;

    if (info->cap_info.wire_len < info->offsets.payload_offset)
    {
        info->offsets.payload_offset = 0;
        ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }
    else if (info->cap_info.caplen < info->offsets.payload_offset)
    {
        info->offsets.payload_offset = 0;
        ret_val = PROTO_HANDLER_NOT_RECORDED_PACKET;
    }
    else
    {
        tcp_header = (struct tcphdr *)data;
        header_len = tcp_header->th_off * TCP_OFFSET_IN_BYTES;

        if (header_len < TCP_HEADER_MIN_LEN)
        {
            info->offsets.payload_offset = 0;
            ret_val = PROTO_HANDLER_CORRUPT_PACKET;
        }
        else
        {
            info->offsets.payload_offset = info->offsets.l4_offset + header_len;
            info->port_info.src_port = ntohs(tcp_header->th_sport);
            info->port_info.dst_port = ntohs(tcp_header->th_dport);
            info->port_info.tcp_flags = tcp_header->th_flags;
        }
    }

    return ret_val;
}

/**
 * @brief Computes the next expected sequence number after a message.
 *
 * Accounts for payload bytes plus one sequence number consumed by SYN or FIN.
 *
 * @param msg The message node to compute from.
 * @return The sequence number of the first byte not yet covered by this message.
 */
static uint32_t calculate_next_seq(message_node_t *msg)
{
    return msg->seq_num + msg->payload_len + ((msg->tcp_flags & (TH_SYN | TH_FIN)) ? 1 : 0);
}

/**
 * @brief Inserts a message into the per-device out-of-order buffer, sorted by sequence distance.
 *
 * Sorts by distance from the expected sequence number to correctly handle
 * TCP sequence number wrap-around. Duplicate segments are dropped, if the
 * incoming segment covers more data than an existing one with the same seq,
 * the existing one is replaced.
 * overlap of message with previos message is handled in the drain function.
 *
 * @param session The session that owns the OOO buffer.
 * @param new_msg The out-of-order message to insert.
 * @param dev_idx The device index (0 or 1) whose buffer to insert into.
 */
static void insert_sorted_ooo(session_node_t *session, message_node_t *new_msg, uint8_t dev_idx)
{
    message_node_t **curr;
    uint32_t expected = session->devices[dev_idx].data.next_expected_seq;
    uint32_t new_dist = new_msg->seq_num - expected;
    uint32_t new_msg_next_seq;
    uint32_t curr_msg_next_seq;

    curr = &(session->devices[dev_idx].ooo_buffer);

    // sorts by distance from expected to deal with loop around:
    // if the distance of the new one is greater than it is after the curr (even if its after the loop around)
    while (*curr != NULL && ((*curr)->seq_num - expected) < new_dist)
    {
        curr = &((*curr)->next);
    }

    // calculate next seq for new msg and current msg for later use in duplicate detection
    if (*curr != NULL)
    {
        new_msg_next_seq = calculate_next_seq(new_msg);
        curr_msg_next_seq = calculate_next_seq(*curr);
    }

    // the new message should be inserted before the current message. Check for duplicates (same seq) and if not duplicate insert.
    if (*curr == NULL || (*curr)->seq_num != new_msg->seq_num)
    {
        new_msg->next = *curr;
        *curr = new_msg;
    }
    // message that has the same seq but is longer => replace the existing one
    else if (new_msg_next_seq - curr_msg_next_seq < (uint32_t)TCP_SEQ_HALF && new_msg_next_seq != curr_msg_next_seq)
    {
        new_msg->next = (*curr)->next;
        free(*curr);
        *curr = new_msg;
    }
    // else: duplicate segment with same coverage - drop new message
    else
    {
        free(new_msg);
    }
}
