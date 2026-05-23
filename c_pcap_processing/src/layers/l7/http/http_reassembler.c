#include "http_reassembler.h"

#include <stdlib.h>
#include <string.h>

#define HEADER_NAME_CONTENT_LENGTH "content-length:"
#define HEADER_NAME_CONTENT_LENGTH_LEN 15

#define HEADER_NAME_TRANSFER_ENCODING "transfer-encoding:"
#define HEADER_NAME_TRANSFER_ENCODING_LEN 18

#define HEADER_NAME_CONNECTION "connection:"
#define HEADER_NAME_CONNECTION_LEN 11


/**
 * @brief Releases a reassembler session state. Safe with NULL.
 *
 * @param session The session to free.
 */
void http_reassembler_session_free(http_reassembler_session_t *session)
{
    if (session != NULL)
    {
        free(session);
    }
}

/**
 * @brief Resets one direction's reassembler back to IDLE, clearing its buffer.
 *
 * @param session The session whose direction to reset.
 * @param dev_idx Direction index (0 or 1).
 */
void http_reassembler_reset_direction(http_reassembler_session_t *session, uint8_t dev_idx)
{
    http_direction_reassembler_t *direction;

    if (session != NULL && dev_idx < DEVICES_IN_FLOW)
    {
        direction = &session->directions[dev_idx];
        direction->current_state = HTTP_REASSEMBLY_STATE_IDLE;
        direction->bytes_in_buffer = 0;
        direction->headers_end_offset = 0;
        direction->declared_content_length = 0;
        direction->body_bytes_consumed = 0;
        direction->current_message_end_offset = 0;
        direction->is_chunked = FALSE;
        direction->connection_close = FALSE;
    }
}

/**
 * @brief Consumes a completed message and slides any pipelined leftover bytes to the front.
 *
 * @param session The session.
 * @param dev_idx Direction whose buffer to consume.
 * @return Number of leftover bytes now sitting at the start of the buffer.
 */
size_t http_reassembler_consume_completed_message(http_reassembler_session_t *session, uint8_t dev_idx)
{
    size_t ret_val;
    http_direction_reassembler_t *direction;
    size_t leftover_count;

    ret_val = 0;

    if (session != NULL && dev_idx < DEVICES_IN_FLOW)
    {
        direction = &session->directions[dev_idx];

        if (direction->current_message_end_offset <= direction->bytes_in_buffer)
        {
            leftover_count = direction->bytes_in_buffer - direction->current_message_end_offset;
        }
        else
        {
            // Defensive: should never happen, but never let leftover wrap.
            leftover_count = 0;
        }

        if (leftover_count > 0)
        {
            memmove(direction->message_buffer, direction->message_buffer + direction->current_message_end_offset, leftover_count);
        }

        direction->current_state = HTTP_REASSEMBLY_STATE_IDLE;
        direction->bytes_in_buffer = leftover_count;
        direction->headers_end_offset = 0;
        direction->declared_content_length = 0;
        direction->body_bytes_consumed = 0;
        direction->current_message_end_offset = 0;
        direction->is_chunked = FALSE;
        direction->connection_close = FALSE;

        ret_val = leftover_count;
    }

    return ret_val;
}

/**
 * @brief Lowercases one byte. ASCII-only, locale-independent.
 *
 * @param c Input byte.
 * @return Lowercase variant if c is an uppercase ASCII letter, else c unchanged.
 */
static uint8_t ascii_to_lower(uint8_t c)
{
    uint8_t ret_val;

    if (c >= 'A' && c <= 'Z')
    {
        ret_val = (uint8_t)(c + ('a' - 'A'));
    }
    else
    {
        ret_val = c;
    }

    return ret_val;
}

/**
 * @brief Case-insensitive memcmp.
 *
 * @param a First buffer.
 * @param b Second buffer.
 * @param len Number of bytes to compare.
 * @return 0 if equal, non-zero otherwise.
 */
static int casefold_memcmp(const uint8_t *a, const char *b, size_t len)
{
    int ret_val;
    size_t byte_idx;

    ret_val = 0;
    byte_idx = 0;

    while (byte_idx < len && ret_val == 0)
    {
        if (ascii_to_lower(a[byte_idx]) != ascii_to_lower((uint8_t)b[byte_idx]))
        {
            ret_val = 1;
        }
        byte_idx++;
    }

    return ret_val;
}

/**
 * @brief Searches the buffer for the headers-end terminator "\r\n\r\n".
 *
 * @param buffer The bytes to scan.
 * @param buffer_len Length of buffer.
 * @param out_offset On success, set to the index of the byte just past "\r\n\r\n".
 * @return TRUE if the terminator was found, FALSE otherwise.
 */
static boolean_e find_headers_end(const uint8_t *buffer,
                                  size_t buffer_len,
                                  size_t *out_offset)
{
    boolean_e ret_val;
    size_t scan_idx;

    ret_val = FALSE;

    if (buffer_len >= HTTP_REASSEMBLY_HEADERS_TERMINATOR_LEN)
    {
        scan_idx = 0;
        while (scan_idx + HTTP_REASSEMBLY_HEADERS_TERMINATOR_LEN <= buffer_len && ret_val == FALSE)
        {
            if (memcmp(buffer + scan_idx,
                       HTTP_REASSEMBLY_HEADERS_TERMINATOR,
                       HTTP_REASSEMBLY_HEADERS_TERMINATOR_LEN) == 0)
            {
                *out_offset = scan_idx + HTTP_REASSEMBLY_HEADERS_TERMINATOR_LEN;
                ret_val = TRUE;
            }
            scan_idx++;
        }
    }

    return ret_val;
}

/**
 * @brief Searches the buffer's tail for the chunked terminator "0\r\n\r\n".
 *
 * Scans from the most recently appended bytes back; only the last few KB
 * need inspection because the terminator must be at the very end of the
 * chunk stream.
 *
 * @param buffer The bytes to scan.
 * @param buffer_len Length of buffer.
 * @param out_terminator_end On TRUE, set to the index one past the terminator.
 * @return TRUE if the terminator is present in the buffer.
 */
static boolean_e find_chunked_terminator(const uint8_t *buffer,
                                         size_t buffer_len,
                                         size_t *out_terminator_end)
{
    boolean_e ret_val;
    size_t scan_idx;

    ret_val = FALSE;

    if (buffer_len >= HTTP_REASSEMBLY_CHUNKED_TERMINATOR_LEN)
    {
        scan_idx = 0;
        while (scan_idx + HTTP_REASSEMBLY_CHUNKED_TERMINATOR_LEN <= buffer_len && ret_val == FALSE)
        {
            if (memcmp(buffer + scan_idx,
                       HTTP_REASSEMBLY_CHUNKED_TERMINATOR,
                       HTTP_REASSEMBLY_CHUNKED_TERMINATOR_LEN) == 0)
            {
                if (out_terminator_end != NULL)
                {
                    *out_terminator_end = scan_idx + HTTP_REASSEMBLY_CHUNKED_TERMINATOR_LEN;
                }
                ret_val = TRUE;
            }
            scan_idx++;
        }
    }

    return ret_val;
}

/**
 * @brief Skips spaces and tabs in a header value (per RFC 7230 OWS rules).
 *
 * @param buffer The bytes to walk.
 * @param start_idx Starting byte index.
 * @param end_idx One past the last valid index.
 * @return Index of the first non-whitespace byte, or end_idx if none.
 */
static size_t skip_optional_whitespace(const uint8_t *buffer, size_t start_idx, size_t end_idx)
{
    size_t cursor;

    cursor = start_idx;
    while (cursor < end_idx && (buffer[cursor] == ' ' || buffer[cursor] == '\t'))
    {
        cursor++;
    }

    return cursor;
}

/**
 * @brief Parses the headers slice and extracts Content-Length, chunked, close.
 *
 * Walks the header bytes line by line (CRLF-delimited), matches the three
 * headers we care about case-insensitively, and populates the direction
 * struct's body-mode fields accordingly.
 *
 * @param direction The direction whose buffer to parse.
 */
static void parse_http_headers_minimal(http_direction_reassembler_t *direction)
{
    const uint8_t *buffer;
    size_t headers_end;
    size_t line_start;
    size_t line_end;
    size_t value_start;
    char content_length_str[32];
    size_t value_len;
    size_t value_idx;
    size_t scan_idx;
    boolean_e found_chunked;
    unsigned long parsed_length;

    buffer = direction->message_buffer;
    headers_end = direction->headers_end_offset;
    line_start = 0;

    while (line_start < headers_end)
    {
        // Find end of current header line (next CRLF).
        line_end = line_start;
        while (line_end + 1 < headers_end &&
               !(buffer[line_end] == '\r' && buffer[line_end + 1] == '\n'))
        {
            line_end++;
        }

        // Content-Length: N
        if ((line_end - line_start) > HEADER_NAME_CONTENT_LENGTH_LEN &&
            casefold_memcmp(buffer + line_start,
                            HEADER_NAME_CONTENT_LENGTH,
                            HEADER_NAME_CONTENT_LENGTH_LEN) == 0)
        {
            value_start = skip_optional_whitespace(buffer,
                                                   line_start + HEADER_NAME_CONTENT_LENGTH_LEN,
                                                   line_end);
            value_len = 0;
            value_idx = value_start;
            while (value_idx < line_end &&
                   value_len < (sizeof(content_length_str) - 1) &&
                   buffer[value_idx] >= '0' && buffer[value_idx] <= '9')
            {
                content_length_str[value_len] = (char)buffer[value_idx];
                value_len++;
                value_idx++;
            }
            content_length_str[value_len] = '\0';
            if (value_len > 0)
            {
                parsed_length = strtoul(content_length_str, NULL, 10);
                direction->declared_content_length = (size_t)parsed_length;
            }
        }
        // Transfer-Encoding: ...chunked...
        else if ((line_end - line_start) > HEADER_NAME_TRANSFER_ENCODING_LEN &&
                 casefold_memcmp(buffer + line_start,
                                 HEADER_NAME_TRANSFER_ENCODING,
                                 HEADER_NAME_TRANSFER_ENCODING_LEN) == 0)
        {
            found_chunked = FALSE;
            scan_idx = line_start + HEADER_NAME_TRANSFER_ENCODING_LEN;
            // Sliding case-insensitive substring search for "chunked" on this line.
            while (scan_idx + 7 <= line_end && found_chunked == FALSE)
            {
                if (casefold_memcmp(buffer + scan_idx, "chunked", 7) == 0)
                {
                    found_chunked = TRUE;
                }
                scan_idx++;
            }
            if (found_chunked == TRUE)
            {
                direction->is_chunked = TRUE;
            }
        }
        // Connection: close
        else if ((line_end - line_start) > HEADER_NAME_CONNECTION_LEN &&
                 casefold_memcmp(buffer + line_start,
                                 HEADER_NAME_CONNECTION,
                                 HEADER_NAME_CONNECTION_LEN) == 0)
        {
            value_start = skip_optional_whitespace(buffer,
                                                   line_start + HEADER_NAME_CONNECTION_LEN,
                                                   line_end);
            if ((line_end - value_start) >= 5 &&
                casefold_memcmp(buffer + value_start, "close", 5) == 0)
            {
                direction->connection_close = TRUE;
            }
        }

        // Advance past this line's CRLF, or to end if no CRLF found.
        if (line_end + 1 < headers_end &&
            buffer[line_end] == '\r' && buffer[line_end + 1] == '\n')
        {
            line_start = line_end + 2;
        }
        else
        {
            line_start = headers_end;
        }
    }
}

/**
 * @brief Returns TRUE if the buffer begins with a method that is "bodyless by default".
 *
 * GET / HEAD / DELETE / OPTIONS without an explicit Content-Length or
 * Transfer-Encoding have no body per RFC 7230 §3.3.3.
 *
 * @param buffer Start of the message buffer (request line).
 * @param buffer_len Length of buffer.
 * @return TRUE if the first token is GET/HEAD/DELETE/OPTIONS.
 */
static boolean_e is_bodyless_default_method(const uint8_t *buffer, size_t buffer_len)
{
    boolean_e ret_val;

    ret_val = FALSE;

    if (buffer_len >= 4 && memcmp(buffer, "GET ", 4) == 0)
    {
        ret_val = TRUE;
    }
    else if (buffer_len >= 5 && memcmp(buffer, "HEAD ", 5) == 0)
    {
        ret_val = TRUE;
    }
    else if (buffer_len >= 7 && memcmp(buffer, "DELETE ", 7) == 0)
    {
        ret_val = TRUE;
    }
    else if (buffer_len >= 8 && memcmp(buffer, "OPTIONS ", 8) == 0)
    {
        ret_val = TRUE;
    }

    return ret_val;
}

/**
 * @brief Appends payload bytes into a direction's buffer with capping.
 *
 * Bytes that exceed HTTP_REASSEMBLY_MAX_MESSAGE_BYTES are silently dropped;
 * the caller detects overflow by inspecting bytes_in_buffer after the call.
 *
 * @param direction The direction to append to.
 * @param payload Source bytes.
 * @param payload_len Source length.
 * @return Number of bytes actually appended.
 */
static size_t append_payload_capped(http_direction_reassembler_t *direction, const uint8_t *payload, size_t payload_len)
{
    size_t space_left;
    size_t to_copy;

    space_left = HTTP_REASSEMBLY_MAX_MESSAGE_BYTES - direction->bytes_in_buffer;

    if (payload_len > space_left)
    {
        to_copy = space_left;
    }
    else
    {
        to_copy = payload_len;
    }

    if (to_copy > 0 && payload != NULL)
    {
        memcpy(direction->message_buffer + direction->bytes_in_buffer, payload, to_copy);
        direction->bytes_in_buffer += to_copy;
    }

    return to_copy;
}

/**
 * @brief Decides the next state after headers are fully collected.
 *
 * Inspects parsed headers and the request line to pick one of:
 *   - COMPLETE                            (no body case)
 *   - COLLECTING_BODY_CONTENT_LENGTH
 *   - COLLECTING_BODY_CHUNKED
 *   - COLLECTING_BODY_UNTIL_CLOSE
 *
 * @param direction The direction whose state to transition.
 */
static void transition_after_headers(http_direction_reassembler_t *direction)
{
    boolean_e is_bodyless;
    size_t terminator_end_in_body;
    size_t needed_total;

    parse_http_headers_minimal(direction);
    is_bodyless = is_bodyless_default_method(direction->message_buffer,
                                             direction->bytes_in_buffer);

    if (direction->is_chunked == TRUE)
    {
        direction->current_state = HTTP_REASSEMBLY_STATE_COLLECTING_BODY_CHUNKED;
        // Check whether the terminator is already present in what we have past the headers.
        if (find_chunked_terminator(
                direction->message_buffer + direction->headers_end_offset,
                direction->bytes_in_buffer - direction->headers_end_offset,
                &terminator_end_in_body) == TRUE)
        {
            direction->current_state = HTTP_REASSEMBLY_STATE_COMPLETE;
            direction->current_message_end_offset =
                direction->headers_end_offset + terminator_end_in_body;
        }
    }
    else if (direction->declared_content_length > 0)
    {
        direction->current_state = HTTP_REASSEMBLY_STATE_COLLECTING_BODY_CONTENT_LENGTH;
        direction->body_bytes_consumed = direction->bytes_in_buffer - direction->headers_end_offset;
        if (direction->body_bytes_consumed >= direction->declared_content_length)
        {
            direction->current_state = HTTP_REASSEMBLY_STATE_COMPLETE;
            needed_total = direction->headers_end_offset + direction->declared_content_length;
            direction->current_message_end_offset = needed_total;
        }
    }
    else if (is_bodyless == TRUE || direction->declared_content_length == 0)
    {
        // GET/HEAD/DELETE/OPTIONS, or any message that explicitly declared zero length.
        direction->current_state = HTTP_REASSEMBLY_STATE_COMPLETE;
        direction->current_message_end_offset = direction->headers_end_offset;
    }
    else if (direction->connection_close == TRUE)
    {
        direction->current_state = HTTP_REASSEMBLY_STATE_COLLECTING_BODY_UNTIL_CLOSE;
    }
    else
    {
        // No length, no chunked, no close: treat as no-body and emit.
        direction->current_state = HTTP_REASSEMBLY_STATE_COMPLETE;
        direction->current_message_end_offset = direction->headers_end_offset;
    }
}

/**
 * @brief Handles state COLLECTING_HEADERS for the current feed call.
 *
 * Appends payload, checks for \r\n\r\n, and on success calls
 * transition_after_headers to pick the next state.
 *
 * @param direction The direction whose state to advance.
 * @param payload Bytes to feed.
 * @param payload_len Length of payload.
 */
static void handle_state_collecting_headers(http_direction_reassembler_t *direction, const uint8_t *payload,size_t payload_len)
{
    size_t end_offset;

    (void)append_payload_capped(direction, payload, payload_len);

    if (find_headers_end(direction->message_buffer, direction->bytes_in_buffer, &end_offset) == TRUE)
    {
        direction->headers_end_offset = end_offset;
        transition_after_headers(direction);
    }
    else if (direction->bytes_in_buffer >= HTTP_REASSEMBLY_MAX_MESSAGE_BYTES)
    {
        direction->current_state = HTTP_REASSEMBLY_STATE_OVERFLOW;
    }
}

/**
 * @brief Handles state COLLECTING_BODY_CONTENT_LENGTH for the current feed.
 *
 * @param direction The direction whose state to advance.
 * @param payload Bytes to feed.
 * @param payload_len Length of payload.
 */
static void handle_state_collecting_body_content_length(http_direction_reassembler_t *direction,
                                                        const uint8_t *payload,
                                                        size_t payload_len)
{
    size_t appended;

    appended = append_payload_capped(direction, payload, payload_len);
    direction->body_bytes_consumed += appended;

    if (direction->body_bytes_consumed >= direction->declared_content_length)
    {
        direction->current_state = HTTP_REASSEMBLY_STATE_COMPLETE;
        direction->current_message_end_offset =
            direction->headers_end_offset + direction->declared_content_length;
    }
    else if (direction->bytes_in_buffer >= HTTP_REASSEMBLY_MAX_MESSAGE_BYTES)
    {
        direction->current_state = HTTP_REASSEMBLY_STATE_OVERFLOW;
    }
}

/**
 * @brief Handles state COLLECTING_BODY_CHUNKED for the current feed.
 *
 * @param direction The direction whose state to advance.
 * @param payload Bytes to feed.
 * @param payload_len Length of payload.
 */
static void handle_state_collecting_body_chunked(http_direction_reassembler_t *direction,
                                                 const uint8_t *payload,
                                                 size_t payload_len)
{
    size_t terminator_end_in_body;

    (void)append_payload_capped(direction, payload, payload_len);

    if (find_chunked_terminator(
            direction->message_buffer + direction->headers_end_offset,
            direction->bytes_in_buffer - direction->headers_end_offset,
            &terminator_end_in_body) == TRUE)
    {
        direction->current_state = HTTP_REASSEMBLY_STATE_COMPLETE;
        direction->current_message_end_offset =
            direction->headers_end_offset + terminator_end_in_body;
    }
    else if (direction->bytes_in_buffer >= HTTP_REASSEMBLY_MAX_MESSAGE_BYTES)
    {
        direction->current_state = HTTP_REASSEMBLY_STATE_OVERFLOW;
    }
}

/**
 * @brief Handles state COLLECTING_BODY_UNTIL_CLOSE for the current feed.
 *
 * Body ends when the TCP FIN flag is observed on this direction's packet,
 * or when the buffer cap is reached.
 *
 * @param direction The direction whose state to advance.
 * @param payload Bytes to feed.
 * @param payload_len Length of payload.
 * @param fin_seen Whether this packet carried the TCP FIN flag.
 */
static void handle_state_collecting_body_until_close(http_direction_reassembler_t *direction,
                                                     const uint8_t *payload,
                                                     size_t payload_len,
                                                     boolean_e fin_seen)
{
    (void)append_payload_capped(direction, payload, payload_len);

    if (fin_seen == TRUE)
    {
        direction->current_state = HTTP_REASSEMBLY_STATE_COMPLETE;
        // Connection-close has no internal length marker; the message is everything we got.
        direction->current_message_end_offset = direction->bytes_in_buffer;
    }
    else if (direction->bytes_in_buffer >= HTTP_REASSEMBLY_MAX_MESSAGE_BYTES)
    {
        direction->current_state = HTTP_REASSEMBLY_STATE_OVERFLOW;
    }
}

/**
 * @brief Feeds one TCP payload chunk for the given direction into the FSM.
 *
 * @param session Session-scoped state.
 * @param dev_idx Direction (0 or 1) the payload was sent from.
 * @param payload Pointer to the TCP payload bytes.
 * @param payload_len Number of valid bytes in payload.
 * @param fin_seen TRUE if this packet carried the TCP FIN flag.
 * @param out_message_bytes On COMPLETE/TRUNCATED, set to the buffer start.
 * @param out_message_len On COMPLETE/TRUNCATED, set to the byte count.
 * @return Feed result describing the new state of the direction.
 */
http_reassembly_feed_result_e http_reassembler_feed_payload(http_reassembler_session_t *session, uint8_t dev_idx, const uint8_t *payload, size_t payload_len, boolean_e fin_seen, const uint8_t **out_message_bytes, size_t *out_message_len)
{
    http_reassembly_feed_result_e ret_val;
    http_direction_reassembler_t *direction;
    boolean_e enter_collecting_headers;

    ret_val = HTTP_REASSEMBLY_FEED_RESULT_NEED_MORE;

    // A NULL payload is allowed if payload_len == 0 (caller is draining prebuffered pipelined bytes that were memmoved into the buffer by consume_completed_message).
    if (session == NULL || dev_idx >= DEVICES_IN_FLOW || out_message_bytes == NULL || out_message_len == NULL || (payload == NULL && payload_len > 0))
    {
        ret_val = HTTP_REASSEMBLY_FEED_RESULT_INVALID_ARG;
    }
    else
    {
        direction = &session->directions[dev_idx];
        enter_collecting_headers = FALSE;

        // IDLE: the next payload bytes begin a new HTTP message. The caller
        // (l7_handler) already confirmed via http_detector that this prefix
        // looks like HTTP, so we just enter COLLECTING_HEADERS and fall through.
        if (direction->current_state == HTTP_REASSEMBLY_STATE_IDLE)
        {
            direction->current_state = HTTP_REASSEMBLY_STATE_COLLECTING_HEADERS;
            enter_collecting_headers = TRUE;
        }

        if (direction->current_state == HTTP_REASSEMBLY_STATE_COLLECTING_HEADERS)
        {
            handle_state_collecting_headers(direction, payload, payload_len);
        }
        else if (direction->current_state == HTTP_REASSEMBLY_STATE_COLLECTING_BODY_CONTENT_LENGTH)
        {
            handle_state_collecting_body_content_length(direction, payload, payload_len);
        }
        else if (direction->current_state == HTTP_REASSEMBLY_STATE_COLLECTING_BODY_CHUNKED)
        {
            handle_state_collecting_body_chunked(direction, payload, payload_len);
        }
        else if (direction->current_state == HTTP_REASSEMBLY_STATE_COLLECTING_BODY_UNTIL_CLOSE)
        {
            handle_state_collecting_body_until_close(direction, payload, payload_len, fin_seen);
        }
        else if (direction->current_state == HTTP_REASSEMBLY_STATE_GIVE_UP)
        {
            ret_val = HTTP_REASSEMBLY_FEED_RESULT_GAVE_UP;
        }

        (void)enter_collecting_headers; // marker var kept for future state-trace logging.

        // Map terminal states to caller-visible result codes.
        if (direction->current_state == HTTP_REASSEMBLY_STATE_COMPLETE)
        {
            *out_message_bytes = direction->message_buffer;
            // Report exactly the current message, any pipelined leftover stays in the buffer for http_reassembler_consume_completed_message to shift.
            if (direction->current_message_end_offset > 0)
            {
                *out_message_len = direction->current_message_end_offset;
            }
            else
            {
                *out_message_len = direction->bytes_in_buffer;
            }
            direction->messages_completed++;
            ret_val = HTTP_REASSEMBLY_FEED_RESULT_MESSAGE_COMPLETE;
        }
        else if (direction->current_state == HTTP_REASSEMBLY_STATE_OVERFLOW)
        {
            *out_message_bytes = direction->message_buffer;
            *out_message_len = direction->bytes_in_buffer;
            ret_val = HTTP_REASSEMBLY_FEED_RESULT_MESSAGE_TRUNCATED;
        }
    }

    return ret_val;
}
