#ifndef HTTP_DETECTOR_H
#define HTTP_DETECTOR_H

#include "common.h"
#include "l7_handler.h"

#define HTTP_DETECTOR_PREFIX_BYTES 16
#define HTTP_DETECTOR_REQUEST_LINE_SCAN_BYTES 512
#define HTTP_DETECTOR_LIKELY_PORT_80   80
#define HTTP_DETECTOR_LIKELY_PORT_8080 8080
#define HTTP_DETECTOR_LIKELY_PORT_8000 8000

typedef enum
{
    HTTP_KIND_NONE = 0,
    HTTP_KIND_REQUEST,
    HTTP_KIND_RESPONSE
} http_kind_e;

/**
 * @brief Check if a payload prefix is the start of an HTTP/1.x message.
 *
 * Looks at the first few bytes of buf. If they match an HTTP/1.x request line
 * verb ("GET ", "POST ", etc.) or the response status line prefix "HTTP/1.",
 * out_kind is set accordingly and a human-readable verb string is returned via
 * out_verb (statically allocated, do not free). Also detects TLS handshakes
 * and the HTTP/2 preface so callers can classify but skip them.
 *
 * @param buf       payload bytes (need not be NUL-terminated)
 * @param len       number of valid bytes in buf
 * @param out_kind  classification of detected HTTP message (REQUEST/RESPONSE/NONE)
 * @param out_verb  on REQUEST: verb token ("GET", "POST", ...); on RESPONSE: "HTTP/1.x"; else NULL
 * @param out_proto coarse L7 protocol guess (HTTP1 / TLS / HTTP2 / UNKNOWN)
 *
 * @return TRUE if a recognizable L7 prefix was found, FALSE otherwise
 */
boolean_e http_detector_check_prefix(const uint8_t *buf,
                                     size_t len,
                                     http_kind_e *out_kind,
                                     const char **out_verb,
                                     l7_protocol_e *out_proto);

/**
 * @brief Returns TRUE if the given TCP port is one of the commonly-used HTTP ports.
 *
 * Used by the hybrid port-vs-payload gate: matches on a likely port let the
 * caller accept HTTP/1.x detection on a verb prefix alone, while non-matching
 * ports require stronger payload confirmation via http_detector_confirm_request_line.
 *
 * @param port TCP port in host byte order.
 * @return TRUE if port is 80, 8080, or 8000.
 */
boolean_e http_detector_is_likely_port(uint16_t port);

/**
 * @brief Confirms an HTTP/1.x request line by finding the version token after the verb.
 *
 * A real HTTP request line ends with "HTTP/1.x\r\n" before the first \r\n.
 * Scans the first line of buf (up to the first \r\n) for the "HTTP/1." token
 * so that a payload like "GET stuff" (matches verb prefix but isn't HTTP) is
 * rejected on non-standard ports.
 *
 * @param buf payload bytes (need not be NUL-terminated).
 * @param len number of valid bytes in buf.
 * @return TRUE if "HTTP/1." appears in the first line, FALSE otherwise.
 */
boolean_e http_detector_confirm_request_line(const uint8_t *buf, size_t len);

#endif
