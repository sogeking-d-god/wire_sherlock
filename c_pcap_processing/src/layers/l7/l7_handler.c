#include "l7_handler.h"
#include "http/http_detector.h"
#include "http/http_signatures.h"
#include "http/http_reassembler.h"
#include "parser.h" // PCAP_PACKET_HEADER_SIZE

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define L7_PREFIX_READ_BYTES HTTP_DETECTOR_PREFIX_BYTES
#define L7_PAYLOAD_READ_CHUNK_BYTES 16384

/**
 * @brief Returns a human-readable string for a given L7 protocol enum value.
 *
 * @param proto The L7 protocol.
 * @return A constant string.
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
 * @brief Formats a flow key into "src:port -> dst:port proto=N".
 *
 * @param key The flow key to format.
 * @param out Output buffer to write into.
 * @param out_len Size of the output buffer.
 */
static void format_flow_key(const flow_key_t *key, char *out, size_t out_len)
{
    char src_buf[INET6_ADDRSTRLEN];
    char dst_buf[INET6_ADDRSTRLEN];

    get_ip_str(&key->src_ip, key->ip_type, src_buf, sizeof(src_buf));
    get_ip_str(&key->dst_ip, key->ip_type, dst_buf, sizeof(dst_buf));

    snprintf(out, out_len, "%s:%u -> %s:%u proto=%u",
             src_buf, key->src_port,
             dst_buf, key->dst_port,
             key->protocol);
}

/**
 * @brief Hybrid port-vs-payload gate: confirms a prefix-detected HTTP/1.x message.
 *
 * On well-known HTTP ports, the prefix match is enough. On any other port,
 * requires the request line to also contain "HTTP/1." within the first line
 * (rejects byte sequences that happen to begin with a verb token by accident).
 *
 * @param key Flow key (for port lookup).
 * @param kind Detected message kind from the prefix detector.
 * @param payload_buf Bytes that were read from the PCAP for this message.
 * @param payload_len Length of payload_buf.
 * @return TRUE if confirmed HTTP/1.x, FALSE if it should be rejected.
 */
static boolean_e confirm_http_via_hybrid_gate(const flow_key_t *key, http_kind_e kind, const uint8_t *payload_buf, size_t payload_len)
{
    boolean_e ret_val;

    if (kind == HTTP_KIND_RESPONSE)
    {
        // Response prefix "HTTP/1." is itself the version token; no further proof needed.
        ret_val = TRUE;
    }
    else if (http_detector_is_likely_port(key->src_port) == TRUE || http_detector_is_likely_port(key->dst_port) == TRUE)
    {
        ret_val = TRUE;
    }
    else
    {
        ret_val = http_detector_confirm_request_line(payload_buf, payload_len);
    }

    return ret_val;
}

/**
 * @brief Logs every match in a chain to stderr with full context.
 *
 * @param matches_head Head of the match list.
 * @param flow_buf Pre-formatted flow key string.
 * @param dev_idx Direction the matched message was sent from.
 */
static void log_match_chain(const http_attack_match_t *matches_head, const char *flow_buf, uint8_t dev_idx)
{
    const http_attack_match_t *current;

    current = matches_head;
    while (current != NULL)
    {
        fprintf(stderr, "[L7][ATTACK] %s dev=%u sig=%s class=%d offset=%zu len=%zu matched='%s'\n",
                flow_buf,
                (unsigned)dev_idx,
                current->signature_id,
                (int)current->attack_class,
                current->match_offset,
                current->match_length,
                current->matched_bytes);
        current = current->next;
    }
}

/**
 * @brief Hands a completed reassembled message buffer to the signature scanner.
 *
 * @param message_bytes Start of the reassembled HTTP message.
 * @param message_len Length of the reassembled message in bytes.
 * @param flow_buf Pre-formatted flow key string for log lines.
 * @param dev_idx Direction the message was sent from.
 * @param out_match_count Incremented by the number of matches found.
 */
static void scan_completed_message(const uint8_t *message_bytes, size_t message_len, const char *flow_buf, uint8_t dev_idx, uint32_t *out_match_count)
{
    http_attack_match_t *matches_head;
    uint32_t local_match_count;
    http_sig_ret_e sig_ret;

    matches_head = NULL;
    local_match_count = 0;

    sig_ret = http_signatures_scan_buffer(message_bytes, message_len, &matches_head, &local_match_count);
    if (sig_ret == HTTP_SIG_SUCCESS && local_match_count > 0)
    {
        log_match_chain(matches_head, flow_buf, dev_idx);
        *out_match_count += local_match_count;
    }
    http_signatures_free_matches(matches_head);
}

/**
 * @brief Reads up to L7_PAYLOAD_READ_CHUNK_BYTES from a message's payload.
 *
 * @param pcap_fd Open PCAP fd.
 * @param msg The message whose payload to read.
 * @param scratch Destination buffer (at least L7_PAYLOAD_READ_CHUNK_BYTES).
 * @param scratch_cap Capacity of scratch.
 * @return Number of bytes read, or -1 on pread error.
 */
static ssize_t read_message_payload(int pcap_fd, const message_node_t *msg, uint8_t *scratch, size_t scratch_cap)
{
    ssize_t ret_val;
    off_t file_off;
    size_t bytes_to_read;

    file_off = (off_t)msg->packet_start_pointer + (off_t)PCAP_PACKET_HEADER_SIZE + (off_t)msg->data_ptr;

    if ((size_t)msg->payload_len > scratch_cap)
    {
        bytes_to_read = scratch_cap;
    }
    else
    {
        bytes_to_read = (size_t)msg->payload_len;
    }

    ret_val = pread(pcap_fd, scratch, bytes_to_read, file_off);

    return ret_val;
}

/**
 * @brief Drains one or more completed HTTP messages from the reassembler.
 *
 * The first call to feed_payload appends payload_buf. If the reassembler
 * reports COMPLETE, the message is scanned and consume_completed_message
 * shifts any pipelined leftover bytes to the front of the buffer. We then
 * loop, feeding zero new bytes, to process those leftover bytes against
 * the next (now-IDLE) state. 
 *
 * TRUNCATED results trigger a hard reset (any leftover bytes are dropped).
 *
 * @param session The session carrying the reassembler.
 * @param key Flow key for the hybrid gate when re-confirming follow-on requests.
 * @param dev_idx Direction the bytes belong to.
 * @param payload_buf First-pass payload to feed (subsequent loop iterations feed 0 bytes).
 * @param payload_len Length of payload_buf.
 * @param fin_seen TRUE if the TCP packet carrying these bytes had the FIN flag.
 * @param flow_buf Pre-formatted flow key string for logging.
 * @param out_hits Incremented for each follow-on pipelined HTTP confirm.
 * @param out_match_count Incremented for each attack match.
 */
static void drain_completed_messages(session_node_t *session, const flow_key_t *key, uint8_t dev_idx, const uint8_t *payload_buf, size_t payload_len,
                                     boolean_e fin_seen, const char *flow_buf, uint32_t *out_hits, uint32_t *out_match_count)
{
    http_reassembler_session_t *reasm_session;
    http_direction_reassembler_t *direction;
    const uint8_t *completed_bytes;
    size_t completed_len;
    http_reassembly_feed_result_e feed_result;
    size_t leftover_count;
    boolean_e draining;
    const uint8_t *feed_bytes;
    size_t feed_bytes_len;
    http_kind_e detected_kind;
    const char *detected_verb;
    l7_protocol_e detected_proto;
    boolean_e confirmed;
    boolean_e leftover_is_http;

    reasm_session = (http_reassembler_session_t *)session->l7;
    direction = NULL;
    draining = (reasm_session != NULL) ? TRUE : FALSE;
    if (reasm_session != NULL)
    {
        direction = &reasm_session->directions[dev_idx];
    }

    // First iteration feeds the freshly-pread payload, subsequent loops feed 0 bytes
    // and process whatever was memmoved to the front of the buffer.
    feed_bytes = payload_buf;
    feed_bytes_len = payload_len;

    while (draining == TRUE)
    {
        completed_bytes = NULL;
        completed_len = 0;
        feed_result = http_reassembler_feed_payload(reasm_session, dev_idx, feed_bytes, feed_bytes_len, fin_seen, &completed_bytes, &completed_len);

        if (feed_result == HTTP_REASSEMBLY_FEED_RESULT_MESSAGE_COMPLETE)
        {
            fprintf(stderr, "[L7][COMPLETE] %s dev=%u msg_bytes=%zu\n", flow_buf, (unsigned)dev_idx, completed_len);
            scan_completed_message(completed_bytes, completed_len, flow_buf, dev_idx, out_match_count);

            // Shift any pipelined leftover bytes to the front. If there are leftover
            // bytes, re-run the detector + hybrid gate on the prebuffered prefix
            // and loop with feed_payload(payload_len=0) so the FSM advances on what
            // is already in the buffer.
            leftover_count = http_reassembler_consume_completed_message(reasm_session, dev_idx);
            if (leftover_count == 0)
            {
                draining = FALSE;
            }
            else
            {
                leftover_is_http = FALSE;
                // Check if the leftover bytes look like another HTTP message.
                if (http_detector_check_prefix(direction->message_buffer, leftover_count, &detected_kind, &detected_verb, &detected_proto) == TRUE && detected_proto == L7_PROTO_HTTP1)
                {
                    confirmed = confirm_http_via_hybrid_gate(key, detected_kind, direction->message_buffer, leftover_count);
                    if (confirmed == TRUE)
                    {
                        leftover_is_http = TRUE;
                        (*out_hits)++;
                        // print the same info as the initial prefix detect log, but with "(pipelined)" and leftover byte count.
                        fprintf(stderr, "[L7][HTTP] %s dev=%u (pipelined) leftover=%zu kind=%s verb=%s\n",
                                flow_buf, (unsigned)dev_idx, leftover_count,
                                (detected_kind == HTTP_KIND_REQUEST) ? "REQUEST" : "RESPONSE",
                                detected_verb ? detected_verb : "?");
                    }
                }

                if (leftover_is_http == TRUE)
                {
                    // Loop again with no new bytes to drive the FSM over the prebuffered leftover.
                    feed_bytes = NULL;
                    feed_bytes_len = 0;
                }
                else
                {
                    // Leftover bytes don't look like a follow-on HTTP message. Drop them.
                    http_reassembler_reset_direction(reasm_session, dev_idx);
                    draining = FALSE;
                }
            }
        }
        else if (feed_result == HTTP_REASSEMBLY_FEED_RESULT_MESSAGE_TRUNCATED)
        {
            fprintf(stderr, "[L7][TRUNCATED] %s dev=%u msg_bytes=%zu (hit 16KB cap)\n", flow_buf, (unsigned)dev_idx, completed_len);
            scan_completed_message(completed_bytes, completed_len, flow_buf, dev_idx, out_match_count);
            http_reassembler_reset_direction(reasm_session, dev_idx);
            draining = FALSE;
        }
        else
        {
            // NEED_MORE / GAVE_UP / INVALID_ARG -> we are done for this packet.
            draining = FALSE;
        }
    }
}

/**
 * @brief Feeds one message's payload into the reassembler and scans on completion.
 *
 * If the reassembler is IDLE, runs the prefix detector + hybrid port gate
 * first to confirm HTTP/1.x. Once confirmed, the bytes are fed and the FSM
 * progresses. On COMPLETE / TRUNCATED, scans the buffer with the signature
 * engine and resets the direction.
 *
 * @param pcap_fd Open PCAP fd.
 * @param key Flow key for hybrid-gate port lookup and logging.
 * @param session Owning session (carries the reassembler state).
 * @param msg The current message to feed.
 * @param payload_buf Scratch buffer the message bytes are read into.
 * @param flow_buf Pre-formatted flow key string for log lines.
 * @param out_hits Incremented for each first-message HTTP confirm.
 * @param out_match_count Incremented for each attack match.
 * @param out_session_proto Set to L7_PROTO_TLS / HTTP2 if detected, so the caller can stop early.
 */
static void feed_message_through_reassembler(int pcap_fd, const flow_key_t *key, session_node_t *session, const message_node_t *msg, uint8_t *payload_buf, const char *flow_buf, uint32_t *out_hits, uint32_t *out_match_count, l7_protocol_e *out_session_proto)
{
    ssize_t got;
    http_kind_e detected_kind;
    const char *detected_verb;
    l7_protocol_e detected_proto;
    boolean_e confirmed;
    http_reassembler_session_t *reasm_session;
    http_direction_reassembler_t *direction;
    boolean_e fin_seen;
    boolean_e processing_active;

    processing_active = TRUE;
    reasm_session = (http_reassembler_session_t *)session->l7;
    direction = NULL;

    // read a tcp payload chunk from the PCAP for this message
    got = read_message_payload(pcap_fd, msg, payload_buf, L7_PAYLOAD_READ_CHUNK_BYTES);
    if (got <= 0)
    {
        processing_active = FALSE;
    }

    if (processing_active == TRUE)
    {
        if (reasm_session != NULL)
        {
            direction = &reasm_session->directions[msg->dev_idx];
        }

        // If this direction is still IDLE, check if the next payload looks like HTTP.
        if (direction == NULL || direction->current_state == HTTP_REASSEMBLY_STATE_IDLE)
        {
            if (http_detector_check_prefix(payload_buf, (size_t)got, &detected_kind, &detected_verb, &detected_proto) == TRUE)
            {
                if (detected_proto == L7_PROTO_TLS || detected_proto == L7_PROTO_HTTP2)
                {
                    *out_session_proto = detected_proto;
                }
                else if (detected_proto == L7_PROTO_HTTP1)
                {
                    confirmed = confirm_http_via_hybrid_gate(key, detected_kind, payload_buf, (size_t)got);
                    if (confirmed == TRUE)
                    {
                        if (reasm_session == NULL)
                        {
                            reasm_session = (http_reassembler_session_t *)calloc(1, sizeof(http_reassembler_session_t));
                            session->l7 = reasm_session;
                        }

                        if (reasm_session != NULL)
                        {
                            direction = &reasm_session->directions[msg->dev_idx];
                            reasm_session->is_http_confirmed = TRUE;
                            (*out_hits)++;
                            fprintf(stderr, "[L7][HTTP] %s dev=%u seq=%u len=%u kind=%s verb=%s\n",
                                    flow_buf,
                                    (unsigned)msg->dev_idx,
                                    msg->seq_num,
                                    msg->payload_len,
                                    (detected_kind == HTTP_KIND_REQUEST) ? "REQUEST" : "RESPONSE",
                                    detected_verb ? detected_verb : "?");
                        }
                        else
                        {
                            fprintf(stderr, "[L7][ERROR] %s dev=%u failed to alloc reassembler session\n",
                                    flow_buf, (unsigned)msg->dev_idx);
                        }
                    }
                }
            }
        }

        // If a reassembler exists for this direction and it has not given up,
        // feed the bytes. The reassembler itself handles the IDLE -> COLLECTING_HEADERS
        // transition on the first feed call.
        if (direction != NULL && direction->current_state != HTTP_REASSEMBLY_STATE_GIVE_UP)
        {
            fin_seen = (msg->tcp_flags & TH_FIN) ? TRUE : FALSE;
            drain_completed_messages(session, key, msg->dev_idx, payload_buf, (size_t)got, fin_seen, flow_buf, out_hits, out_match_count);
        }
    }
}

/**
 * @brief Walks every message in a session, feeding payloads to the reassembler.
 *
 * Stops early if the session is classified as TLS or HTTP/2.
 *
 * @param pcap_fd Open PCAP fd.
 * @param key The flow key (used for logging + hybrid port gate).
 * @param session The session to walk.
 * @param out_hits Incremented for each HTTP message start confirmed.
 * @param out_match_count Incremented for each attack match.
 */
static void process_session(int pcap_fd, const flow_key_t *key, session_node_t *session, uint32_t *out_hits, uint32_t *out_match_count)
{
    message_node_t *msg;
    uint8_t payload_buf[L7_PAYLOAD_READ_CHUNK_BYTES];
    char flow_buf[128];
    l7_protocol_e session_proto;

    session_proto = L7_PROTO_UNKNOWN;
    format_flow_key(key, flow_buf, sizeof(flow_buf));
    msg = session->messages.head;

    while (msg != NULL && session_proto != L7_PROTO_TLS && session_proto != L7_PROTO_HTTP2)
    {
        if (msg->payload_len > 0)
        {
            feed_message_through_reassembler(pcap_fd, key, session, msg, payload_buf, flow_buf, out_hits, out_match_count, &session_proto);
        }
        msg = msg->next;
    }

    if (session_proto == L7_PROTO_TLS || session_proto == L7_PROTO_HTTP2)
    {
        fprintf(stderr, "[L7][%s] %s (detected, not parsed)\n", l7_proto_str(session_proto), flow_buf);
    }
}

/**
 * @brief Runs the L7 sweep over all TCP flows in the table.
 *
 * @param table The flow table to sweep.
 * @param pcap_path Path to the PCAP file to read payload bytes from.
 * @return L7_HANDLER_SUCCESS, or an error code on failure.
 */
l7_handler_ret_e l7_handler_run(flow_table_t *table, const char *pcap_path)
{
    l7_handler_ret_e ret_val;
    int pcap_fd;
    uint32_t bucket_idx;
    flow_node_t *flow;
    session_node_t *session;
    uint32_t total_hits;
    uint32_t scanned_sessions;
    uint32_t total_matches;
    http_sig_ret_e sig_init_ret;

    ret_val = L7_HANDLER_SUCCESS;
    total_hits = 0;
    scanned_sessions = 0;
    total_matches = 0;

    if (table == NULL || pcap_path == NULL)
    {
        ret_val = L7_HANDLER_NULL_ARG;
    }
    else
    {
        sig_init_ret = http_signatures_init();
        if (sig_init_ret != HTTP_SIG_SUCCESS)
        {
            fprintf(stderr, "[L7] failed to init attack signatures (code=%d)\n", (int)sig_init_ret);
            ret_val = L7_HANDLER_SIG_INIT_FAILED;
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
                    while (flow != NULL)
                    {
                        if (flow->key.protocol == IPPROTO_TCP)
                        {
                            session = flow->first_session;
                            while (session != NULL)
                            {
                                scanned_sessions++;
                                process_session(pcap_fd, &flow->key, session, &total_hits, &total_matches);
                                session = session->next;
                            }
                        }
                        flow = flow->next;
                    }
                }

                close(pcap_fd);

                fprintf(stderr,
                        "[L7] sweep done: scanned %u TCP sessions, detected %u HTTP/1.x message starts, %u attack signature matches\n",
                        scanned_sessions, total_hits, total_matches);
            }
        }
    }

    return ret_val;
}
