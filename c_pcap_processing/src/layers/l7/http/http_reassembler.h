#ifndef HTTP_REASSEMBLER_H
#define HTTP_REASSEMBLER_H

#include "common.h"
#include "flow_table.h"

// Hard cap per direction per message. Bounds total memory and DoS surface.
#define HTTP_REASSEMBLY_MAX_MESSAGE_BYTES 16384

// Max length of a single HTTP header line we will inspect when looking up
// Content-Length / Transfer-Encoding / Connection.
#define HTTP_REASSEMBLY_HEADER_LINE_MAX_LEN 4096

#define HTTP_REASSEMBLY_HEADERS_TERMINATOR "\r\n\r\n"
#define HTTP_REASSEMBLY_HEADERS_TERMINATOR_LEN 4

// "0" followed by CRLF then CRLF: end of a chunked transfer's last chunk.
#define HTTP_REASSEMBLY_CHUNKED_TERMINATOR "0\r\n\r\n"
#define HTTP_REASSEMBLY_CHUNKED_TERMINATOR_LEN 5

typedef enum
{
    HTTP_REASSEMBLY_STATE_IDLE = 0,
    HTTP_REASSEMBLY_STATE_COLLECTING_HEADERS,
    HTTP_REASSEMBLY_STATE_COLLECTING_BODY_CONTENT_LENGTH,
    HTTP_REASSEMBLY_STATE_COLLECTING_BODY_CHUNKED,
    HTTP_REASSEMBLY_STATE_COLLECTING_BODY_UNTIL_CLOSE,
    HTTP_REASSEMBLY_STATE_COMPLETE,
    HTTP_REASSEMBLY_STATE_OVERFLOW,
    HTTP_REASSEMBLY_STATE_GIVE_UP
} http_reassembly_state_e;

typedef enum
{
    HTTP_REASSEMBLY_FEED_RESULT_NEED_MORE = 0,
    HTTP_REASSEMBLY_FEED_RESULT_MESSAGE_COMPLETE,
    HTTP_REASSEMBLY_FEED_RESULT_MESSAGE_TRUNCATED,
    HTTP_REASSEMBLY_FEED_RESULT_GAVE_UP,
    HTTP_REASSEMBLY_FEED_RESULT_INVALID_ARG
} http_reassembly_feed_result_e;

typedef struct
{
    http_reassembly_state_e current_state;
    uint8_t message_buffer[HTTP_REASSEMBLY_MAX_MESSAGE_BYTES];
    size_t bytes_in_buffer;
    size_t headers_end_offset;        // index just past the "\r\n\r\n"
    size_t declared_content_length;
    size_t body_bytes_consumed;
    size_t current_message_end_offset; // byte just past the end of the just-completed message
    boolean_e is_chunked;
    boolean_e connection_close;
    uint32_t messages_completed;
} http_direction_reassembler_t;

typedef struct l7_session_state_s
{
    http_direction_reassembler_t directions[DEVICES_IN_FLOW];
    boolean_e is_http_confirmed;
    uint32_t total_attack_matches;
} http_reassembler_session_t;

/**
 * @brief Releases a reassembler session state. Safe with NULL.
 *
 * The struct has no inner heap allocations (the message buffer is inline),
 * so this is a single free.
 *
 * @param session The session to free.
 */
void http_reassembler_session_free(http_reassembler_session_t *session);

/**
 * @brief Feeds one TCP payload chunk for the given direction into the FSM.
 *
 * Appends payload bytes to the direction's reassembly buffer, advances the
 * state machine per RFC 7230 termination rules (Content-Length, chunked,
 * Connection: close, no-body), and reports whether a complete HTTP message
 * is now sitting in the buffer ready to be scanned.
 *
 * On HTTP_REASSEMBLY_FEED_RESULT_MESSAGE_COMPLETE the caller should:
 *   1. Read out_message_bytes from direction->message_buffer with length
 *      out_message_len.
 *   2. Call http_reassembler_reset_direction(session, dev_idx) before
 *      feeding the next payload for the same direction.
 *
 * @param session Session-scoped state (must not be NULL).
 * @param dev_idx Direction (0 or 1) the payload was sent from.
 * @param payload Pointer to the TCP payload bytes (not NUL-terminated).
 * @param payload_len Number of valid bytes in payload.
 * @param fin_seen TRUE if this packet carried the TCP FIN flag for the sender.
 * @param out_message_bytes On COMPLETE / TRUNCATED, set to the start of the
 *                          reassembled buffer.
 * @param out_message_len On COMPLETE / TRUNCATED, set to the number of valid
 *                        bytes in out_message_bytes.
 * @return Feed result describing whether the caller now has a complete message,
 *         needs more data, or the direction has given up.
 */
http_reassembly_feed_result_e http_reassembler_feed_payload(
    http_reassembler_session_t *session,
    uint8_t dev_idx,
    const uint8_t *payload,
    size_t payload_len,
    boolean_e fin_seen,
    const uint8_t **out_message_bytes,
    size_t *out_message_len);

/**
 * @brief Resets one direction's reassembler back to IDLE, clearing its buffer.
 *
 * Use this for TRUNCATED / non-pipelined COMPLETE situations where any
 * remaining bytes in the buffer should be dropped (overflow resync, or
 * caller explicitly wants a hard reset).
 *
 * @param session The session whose direction to reset.
 * @param dev_idx Direction index (0 or 1).
 */
void http_reassembler_reset_direction(http_reassembler_session_t *session,
                                      uint8_t dev_idx);

/**
 * @brief Consumes a just-completed message and shifts any leftover bytes to the front.
 *
 * Use this after a HTTP_REASSEMBLY_FEED_RESULT_MESSAGE_COMPLETE result. The
 * function uses memmove to slide any bytes following current_message_end_offset
 * to the start of the buffer, sets bytes_in_buffer to the count of leftover
 * bytes, resets per-message metadata fields, and returns the direction to IDLE.
 * The leftover bytes belong to a pipelined follow-on HTTP message and the
 * caller should re-evaluate them (run the detector / hybrid gate, then call
 * feed_payload with payload_len=0 to process the prebuffered bytes).
 *
 * @param session The session.
 * @param dev_idx Direction whose buffer to consume.
 * @return Number of leftover bytes now sitting at the start of the buffer.
 */
size_t http_reassembler_consume_completed_message(http_reassembler_session_t *session,
                                                  uint8_t dev_idx);

#endif
