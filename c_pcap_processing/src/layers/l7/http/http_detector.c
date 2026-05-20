#include "http_detector.h"

typedef struct
{
    const char *verb;
    size_t len;
} http_verb_entry_t;

static const http_verb_entry_t HTTP_VERBS[] = {
    {"GET ",     4},
    {"POST ",    5},
    {"PUT ",     4},
    {"DELETE ",  7},
    {"HEAD ",    5},
    {"OPTIONS ", 8},
    {"PATCH ",   6},
    {"CONNECT ", 8},
    {"TRACE ",   6}
};

static const size_t HTTP_VERB_COUNT = sizeof(HTTP_VERBS) / sizeof(HTTP_VERBS[0]);

static const char HTTP_RESPONSE_PREFIX[] = "HTTP/1.";
static const size_t HTTP_RESPONSE_PREFIX_LEN = sizeof(HTTP_RESPONSE_PREFIX) - 1;

// HTTP/2 client connection preface (RFC 7540 sec 3.5)
static const char HTTP2_PREFACE[] = "PRI * HTTP/2.0";
static const size_t HTTP2_PREFACE_LEN = sizeof(HTTP2_PREFACE) - 1;

/**
 * @brief Inspects the first bytes of a payload to detect the L7 protocol.
 *
 * Checks for HTTP/1.x request verbs, HTTP/1.x status lines, the HTTP/2
 * client preface, and TLS handshake record headers. On a match the relevant
 * out parameters are populated.
 *
 * @param buf Pointer to the payload prefix bytes.
 * @param len Number of bytes available in buf.
 * @param out_kind Set to HTTP_KIND_REQUEST or HTTP_KIND_RESPONSE if HTTP/1.x is detected.
 * @param out_verb Set to the matched verb or prefix string if HTTP/1.x is detected.
 * @param out_proto Set to the detected L7 protocol enum value.
 * @return TRUE if any protocol was detected, FALSE otherwise.
 */
boolean_e http_detector_check_prefix(const uint8_t *buf,
                                     size_t len,
                                     http_kind_e *out_kind,
                                     const char **out_verb,
                                     l7_protocol_e *out_proto)
{
    boolean_e found = FALSE;
    size_t verb_idx;

    if (out_kind)  { *out_kind  = HTTP_KIND_NONE; }
    if (out_verb)  { *out_verb  = NULL; }
    if (out_proto) { *out_proto = L7_PROTO_UNKNOWN; }

    if (!buf || len == 0)
    {
        found = FALSE;
    }
    else
    {
        // HTTP/1.x request line
        verb_idx = 0;
        while (verb_idx < HTTP_VERB_COUNT && !found)
        {
            if (len >= HTTP_VERBS[verb_idx].len &&
                memcmp(buf, HTTP_VERBS[verb_idx].verb, HTTP_VERBS[verb_idx].len) == 0)
            {
                if (out_kind)  { *out_kind  = HTTP_KIND_REQUEST; }
                if (out_verb)  { *out_verb  = HTTP_VERBS[verb_idx].verb; }
                if (out_proto) { *out_proto = L7_PROTO_HTTP1; }
                found = TRUE;
            }

            verb_idx++;
        }

        // HTTP/1.x status line
        if (!found &&
            len >= HTTP_RESPONSE_PREFIX_LEN &&
            memcmp(buf, HTTP_RESPONSE_PREFIX, HTTP_RESPONSE_PREFIX_LEN) == 0)
        {
            if (out_kind)  { *out_kind  = HTTP_KIND_RESPONSE; }
            if (out_verb)  { *out_verb  = HTTP_RESPONSE_PREFIX; }
            if (out_proto) { *out_proto = L7_PROTO_HTTP1; }
            found = TRUE;
        }

        // HTTP/2 preface (we identify but don't parse)
        if (!found &&
            len >= HTTP2_PREFACE_LEN &&
            memcmp(buf, HTTP2_PREFACE, HTTP2_PREFACE_LEN) == 0)
        {
            if (out_proto) { *out_proto = L7_PROTO_HTTP2; }
            found = TRUE;
        }

        // TLS handshake record: byte0=0x16 (handshake), byte1=0x03 (TLS major),
        // byte2 in {0x00, 0x01, 0x02, 0x03, 0x04} (TLS 1.0..1.3 / SSL 3.0)
        if (!found &&
            len >= 3 &&
            buf[0] == 0x16 &&
            buf[1] == 0x03 &&
            buf[2] <= 0x04)
        {
            if (out_proto) { *out_proto = L7_PROTO_TLS; }
            found = TRUE;
        }
    }

    return found;
}
