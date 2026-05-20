#include "l7_handler.h"
#include "http/http_detector.h"
#include "parser.h" // PCAP_PACKET_HEADER_SIZE

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define L7_PREFIX_READ_BYTES HTTP_DETECTOR_PREFIX_BYTES

/**
 * @brief Returns a human-readable string for a given L7 protocol enum value.
 *
 * @param proto The L7 protocol to stringify.
 * @return A constant string like "HTTP/1", "TLS", etc.
 */
static const char *l7_proto_str(l7_protocol_e proto)
{
    const char *proto_str;

    switch (proto)
    {
        case L7_PROTO_HTTP1:
            proto_str = "HTTP/1";
            break;
        case L7_PROTO_TLS:
            proto_str = "TLS";
            break;
        case L7_PROTO_HTTP2:
            proto_str = "HTTP/2";
            break;
        case L7_PROTO_NOT_TEXT:
            proto_str = "binary";
            break;
        case L7_PROTO_UNKNOWN:
        default:
            proto_str = "unknown";
            break;
    }

    return proto_str;
}

/**
 * @brief Formats a flow key into a human-readable "src:port -> dst:port proto=N" string.
 *
 * @param key The flow key to format.
 * @param out Output buffer to write into.
 * @param out_len Size of the output buffer.
 */
static void format_flow_key(const flow_key_t *key, char *out, size_t out_len)
{
    char src_buf[INET6_ADDRSTRLEN];
    char dst_buf[INET6_ADDRSTRLEN];

    // get_ip_str writes into the buffer and returns it
    get_ip_str(&key->src_ip, key->ip_type, src_buf, sizeof(src_buf));
    get_ip_str(&key->dst_ip, key->ip_type, dst_buf, sizeof(dst_buf));

    // ports in flow_key_t are stored in host byte order (see tcp_handler.c)
    snprintf(out, out_len, "%s:%u -> %s:%u proto=%u",
             src_buf, key->src_port,
             dst_buf, key->dst_port,
             key->protocol);
}

/**
 * @brief Scans all messages in a session and logs detected L7 protocol boundaries.
 *
 * Reads the first bytes of each message's payload directly from the PCAP file
 * and runs the HTTP/TLS/HTTP2 detector on them. Logs HTTP/1.x message starts
 * and stops scanning a session as soon as TLS or HTTP/2 is detected.
 *
 * @param pcap_fd Open file descriptor for the PCAP file.
 * @param key The flow key, used for log formatting.
 * @param session The session whose messages to scan.
 * @param out_hits Incremented for each detected HTTP/1.x message start.
 */
static void process_session(int pcap_fd, const flow_key_t *key, session_node_t *session, uint32_t *out_hits)
{
    message_node_t *msg;
    uint8_t prefix[L7_PREFIX_READ_BYTES];
    ssize_t got;
    off_t file_off;
    http_kind_e kind;
    const char *verb;
    l7_protocol_e proto;
    char flow_buf[128];
    boolean_e stop_scanning;

    stop_scanning = FALSE;
    msg = session->messages.head;

    while (msg && !stop_scanning)
    {
        if (msg->payload_len > 0)
        {
            // packet_start_pointer points at the 16-byte pcap record header.
            // data_ptr is the payload offset measured from the start of the packet
            file_off = (off_t)msg->packet_start_pointer + (off_t)PCAP_PACKET_HEADER_SIZE + (off_t)msg->data_ptr;

            got = pread(pcap_fd, prefix, sizeof(prefix), file_off);

            if (got < 0)
            {
                fprintf(stderr, "[L7] pread failed at offset %lld: %s\n",
                        (long long)file_off, strerror(errno));
            }
            else if (got > 0)
            {
                if (http_detector_check_prefix(prefix, (size_t)got, &kind, &verb, &proto))
                {
                    // Only log HTTP/1.x message starts in this milestone.
                    // TLS / HTTP/2 are detected but we don't analyze them.
                    if (proto == L7_PROTO_HTTP1)
                    {
                        format_flow_key(key, flow_buf, sizeof(flow_buf));
                        fprintf(stderr,
                                "[L7][HTTP] %s dev=%u seq=%u len=%u file_off=%lld kind=%s verb=%s\n",
                                flow_buf,
                                (unsigned)msg->dev_idx,
                                msg->seq_num,
                                msg->payload_len,
                                (long long)file_off,
                                (kind == HTTP_KIND_REQUEST) ? "REQUEST" : "RESPONSE",
                                verb ? verb : "?");
                        (*out_hits)++;
                    }
                    else if (proto == L7_PROTO_TLS || proto == L7_PROTO_HTTP2)
                    {
                        // Mark and move on.
                        format_flow_key(key, flow_buf, sizeof(flow_buf));
                        fprintf(stderr, "[L7][%s] %s dev=%u (detected, not parsed)\n",
                                l7_proto_str(proto), flow_buf, (unsigned)msg->dev_idx);
                        stop_scanning = TRUE;
                    }
                }
            }
        }

        msg = msg->next;
    }
}

/**
 * @brief Runs the L7 detection sweep over all TCP flows in the table.
 *
 * Opens the PCAP file, iterates over every TCP session in the flow table,
 * and calls process_session on each. Logs summary statistics when done.
 *
 * @param table The flow table to sweep.
 * @param pcap_path Path to the PCAP file to read payload bytes from.
 * @return L7_HANDLER_SUCCESS, or an error code on failure.
 */
l7_handler_ret_e l7_handler_run(flow_table_t *table, const char *pcap_path)
{
    l7_handler_ret_e ret_val = L7_HANDLER_SUCCESS;
    int pcap_fd;
    uint32_t bucket_idx;
    flow_node_t *flow;
    session_node_t *session;
    uint32_t total_hits = 0;
    uint32_t scanned_sessions = 0;

    if (!table || !pcap_path)
    {
        ret_val = L7_HANDLER_NULL_ARG;
    }
    else
    {
        pcap_fd = open(pcap_path, O_RDONLY);

        if (pcap_fd < 0)
        {
            fprintf(stderr, "[L7] failed to open pcap %s: %s\n", pcap_path, strerror(errno));
            ret_val = L7_HANDLER_PCAP_OPEN_FAILED;
        }
        else
        {
            fprintf(stderr, "[L7] starting L7 sweep over flow table (file=%s)\n", pcap_path);

            for (bucket_idx = 0; bucket_idx < FLOW_HASH_SIZE; bucket_idx++)
            {
                flow = table->buckets[bucket_idx];

                while (flow)
                {
                    // Only TCP carries HTTP/1.x and TLS in our scope.
                    if (flow->key.protocol == IPPROTO_TCP)
                    {
                        session = flow->first_session;

                        while (session)
                        {
                            scanned_sessions++;
                            process_session(pcap_fd, &flow->key, session, &total_hits);
                            session = session->next;
                        }
                    }

                    flow = flow->next;
                }
            }

            close(pcap_fd);

            fprintf(stderr, "[L7] sweep done: scanned %u TCP sessions, detected %u HTTP/1.x message starts\n",
                    scanned_sessions, total_hits);
        }
    }

    return ret_val;
}

/**
 * @brief Frees all memory owned by an l7_session_state_t.
 *
 * @param state Pointer to the L7 session state to free. Safe to call with NULL.
 */
void l7_session_state_free(l7_session_state_t *state)
{
    http_message_record_t *cur;
    http_message_record_t *next;

    if (state)
    {
        cur = state->messages_dev0;
        while (cur)
        {
            next = cur->next;
            free(cur);
            cur = next;
        }

        cur = state->messages_dev1;
        while (cur)
        {
            next = cur->next;
            free(cur);
            cur = next;
        }

        free(state);
    }
}
