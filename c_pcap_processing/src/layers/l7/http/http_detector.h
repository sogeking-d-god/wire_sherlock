#ifndef HTTP_DETECTOR_H
#define HTTP_DETECTOR_H

#include "common.h"
#include "l7_handler.h"

#define HTTP_DETECTOR_PREFIX_BYTES 16

typedef enum {
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

#endif
