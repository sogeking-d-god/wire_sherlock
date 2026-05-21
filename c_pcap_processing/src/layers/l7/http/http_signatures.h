#ifndef HTTP_SIGNATURES_H
#define HTTP_SIGNATURES_H

#include <regex.h>
#include <sys/types.h>

#include "common.h"

#define HTTP_SIG_MATCHED_BYTES_LEN 64

typedef enum
{
    HTTP_ATTACK_CLASS_NONE = 0,
    HTTP_ATTACK_CLASS_SQLI,
    HTTP_ATTACK_CLASS_XSS
} http_attack_class_e;

typedef struct
{
    const char *signature_id;
    http_attack_class_e attack_class;
    const char *pattern_source;
    regex_t compiled_pattern;
    boolean_e is_compiled;
} http_signature_entry_t;

typedef struct http_attack_match
{
    const char *signature_id;
    http_attack_class_e attack_class;
    size_t match_offset;
    size_t match_length;
    char matched_bytes[HTTP_SIG_MATCHED_BYTES_LEN];
    struct http_attack_match *next;
} http_attack_match_t;

typedef enum
{
    HTTP_SIG_SUCCESS = 0,
    HTTP_SIG_NULL_ARG = -1,
    HTTP_SIG_REGCOMP_FAILED = -2,
    HTTP_SIG_ALLOC_FAILED = -3
} http_sig_ret_e;

/**
 * @brief Compiles all built-in attack signatures into the static registry.
 *
 * Must be called once at engine startup, before any call to
 * http_signatures_scan_buffer. Idempotent — repeated calls are no-ops.
 *
 * @return HTTP_SIG_SUCCESS on success, HTTP_SIG_REGCOMP_FAILED if any
 *         regex_t failed to compile (partial state is rolled back).
 */
http_sig_ret_e http_signatures_init(void);

/**
 * @brief Releases all regex_t resources held by the static signature registry.
 *
 * Must be called at engine shutdown. Safe to call multiple times.
 */
void http_signatures_destroy(void);

/**
 * @brief Scans a reassembled HTTP buffer for all known attack signatures.
 *
 * Runs every compiled pattern against the buffer and appends one
 * http_attack_match_t to out_matches_head for each hit. The caller owns
 * the resulting linked list and must free it with
 * http_signatures_free_matches.
 *
 * @param buffer Reassembled HTTP header bytes (need not be NUL-terminated).
 * @param buffer_len Number of valid bytes in buffer.
 * @param out_matches_head Output: head pointer of the appended match list.
 *                         Caller passes &NULL on first call; subsequent
 *                         calls append to the existing list.
 * @param out_match_count Output: number of new matches appended.
 *
 * @return HTTP_SIG_SUCCESS on a clean scan (zero or more matches),
 *         HTTP_SIG_NULL_ARG / HTTP_SIG_ALLOC_FAILED on errors.
 */
http_sig_ret_e http_signatures_scan_buffer(const uint8_t *buffer,
                                           size_t buffer_len,
                                           http_attack_match_t **out_matches_head,
                                           uint32_t *out_match_count);

/**
 * @brief Frees a linked list of http_attack_match_t records.
 *
 * @param matches_head Head of the list to free. Safe with NULL.
 */
void http_signatures_free_matches(http_attack_match_t *matches_head);

#endif