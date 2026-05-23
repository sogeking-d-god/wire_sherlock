#include "http_signatures.h"

#include <stdlib.h>
#include <string.h>

#define HTTP_SIG_SCRATCH_BUFFER_LEN 65536

static http_signature_entry_t g_http_signature_registry[] =
{
    {
        "sqli_select_from",
        HTTP_ATTACK_CLASS_SQLI,
        // Match a space, tab, %20, or + between tokens to catch both literal
        "select([[:space:]]|%20|\\+).*from",
        {0},
        FALSE
    },
    {
        "sqli_union_select",
        HTTP_ATTACK_CLASS_SQLI,
        "union([[:space:]]|%20|\\+).*select",
        {0},
        FALSE
    },
    {
        "xss_script_tag",
        HTTP_ATTACK_CLASS_XSS,
        "<script>.*</script>",
        {0},
        FALSE
    },
    {
        "xss_javascript_uri",
        HTTP_ATTACK_CLASS_XSS,
        "javascript:",
        {0},
        FALSE
    }
};

static const size_t g_http_signature_count =
    sizeof(g_http_signature_registry) / sizeof(g_http_signature_registry[0]);

static boolean_e g_http_signatures_initialized = FALSE;

/**
 * @brief Rolls back partial regex compilation when init fails midway.
 *
 * Walks the registry up to compiled_up_to and frees every entry whose
 * is_compiled flag is set. Used internally by http_signatures_init when
 * a later regcomp call fails after earlier ones succeeded.
 *
 * @param compiled_up_to One past the last index that may need freeing.
 */
static void http_signatures_rollback_compiled(size_t compiled_up_to)
{
    size_t entry_idx;

    for (entry_idx = 0; entry_idx < compiled_up_to; entry_idx++)
    {
        if (g_http_signature_registry[entry_idx].is_compiled == TRUE)
        {
            regfree(&g_http_signature_registry[entry_idx].compiled_pattern);
            g_http_signature_registry[entry_idx].is_compiled = FALSE;
        }
    }
}

/**
 * @brief Compiles all built-in attack signatures into the static registry.
 *
 * Iterates the registry and calls regcomp on each pattern with
 * REG_EXTENDED | REG_ICASE so the patterns are case-insensitive POSIX
 * extended regular expressions. If any pattern fails to compile, all
 * previously-compiled patterns are freed and an error is returned.
 *
 * @return HTTP_SIG_SUCCESS on success, HTTP_SIG_REGCOMP_FAILED on failure.
 */
http_sig_ret_e http_signatures_init(void)
{
    http_sig_ret_e ret_val;
    size_t entry_idx;
    boolean_e compile_failed;
    int regcomp_ret;

    ret_val = HTTP_SIG_SUCCESS;
    entry_idx = 0;
    compile_failed = FALSE;

    if (g_http_signatures_initialized == TRUE)
    {
        ret_val = HTTP_SIG_SUCCESS;
    }
    else
    {
        while (entry_idx < g_http_signature_count && compile_failed == FALSE)
        {
            // flags: case-insensitive, extended regex syntax with spetial chars
            regcomp_ret = regcomp(&g_http_signature_registry[entry_idx].compiled_pattern, g_http_signature_registry[entry_idx].pattern_source, REG_EXTENDED | REG_ICASE);
            if (regcomp_ret == 0)
            {
                g_http_signature_registry[entry_idx].is_compiled = TRUE;
                entry_idx++;
            }
            else
            {
                fprintf(stderr,
                        "[L7][sig] regcomp failed for signature '%s' pattern '%s' (code=%d)\n",
                        g_http_signature_registry[entry_idx].signature_id,
                        g_http_signature_registry[entry_idx].pattern_source,
                        regcomp_ret);
                compile_failed = TRUE;
            }
        }

        if (compile_failed == TRUE)
        {
            http_signatures_rollback_compiled(entry_idx);
            ret_val = HTTP_SIG_REGCOMP_FAILED;
        }
        else
        {
            g_http_signatures_initialized = TRUE;
        }
    }

    return ret_val;
}

/**
 * @brief Releases all regex_t resources held by the static signature registry.
 *
 * Safe to call when uninitialized or after a previous destroy.
 */
void http_signatures_destroy(void)
{
    if (g_http_signatures_initialized == TRUE)
    {
        http_signatures_rollback_compiled(g_http_signature_count);
        g_http_signatures_initialized = FALSE;
    }
}

/**
 * @brief Copies buffer into a NUL-terminated scratch buffer for regexec.
 *
 * Embedded NUL bytes (0x00) inside the payload are replaced with space (0x20) so POSIX regexec sees the entire buffer rather than  at the first NUL.
 *
 * @param src Input bytes (need not be NUL-terminated).
 * @param src_len Number of valid bytes in src.
 * @param dst Output buffer of at least HTTP_SIG_SCRATCH_BUFFER_LEN bytes.
 * @return Number of bytes copied (excluding the trailing NUL).
 */
static size_t http_signatures_copy_for_regex(const uint8_t *src, size_t src_len, char *dst)
{
    size_t copy_len;
    size_t byte_idx;

    if (src_len > (HTTP_SIG_SCRATCH_BUFFER_LEN - 1))
    {
        copy_len = HTTP_SIG_SCRATCH_BUFFER_LEN - 1;
    }
    else
    {
        copy_len = src_len;
    }

    for (byte_idx = 0; byte_idx < copy_len; byte_idx++)
    {
        if (src[byte_idx] == 0x00)
        {
            dst[byte_idx] = 0x20;
        }
        else
        {
            dst[byte_idx] = (char)src[byte_idx];
        }
    }
    dst[copy_len] = '\0';

    return copy_len;
}

/**
 * @brief Creates a match record for a single regex hit.
 *
 * Copies up to HTTP_SIG_MATCHED_BYTES_LEN-1 bytes of the matched substring into the record so the caller can
 * see exactly what triggered the signature.
 *
 * @param entry Source signature entry that produced the hit.
 * @param scratch The NUL-terminated buffer that was scanned.
 * @param match_start_offset Byte offset of the match within scratch.
 * @param match_len Length of the matched substring.
 * @return Allocated match record, or NULL on allocation failure.
 */
static http_attack_match_t *http_signatures_make_match(const http_signature_entry_t *entry, const char *scratch, size_t match_start_offset, size_t match_len)
{
    http_attack_match_t *new_match;
    size_t bytes_to_copy;

    new_match = (http_attack_match_t *)calloc(1, sizeof(http_attack_match_t));

    if (new_match != NULL)
    {
        new_match->signature_id = entry->signature_id;
        new_match->attack_class = entry->attack_class;
        new_match->match_offset = match_start_offset;
        new_match->match_length = match_len;
        new_match->next = NULL;

        if (match_len >= HTTP_SIG_MATCHED_BYTES_LEN)
        {
            bytes_to_copy = HTTP_SIG_MATCHED_BYTES_LEN - 1;
        }
        else
        {
            bytes_to_copy = match_len;
        }

        memcpy(new_match->matched_bytes, scratch + match_start_offset, bytes_to_copy);
        new_match->matched_bytes[bytes_to_copy] = '\0';
    }

    return new_match;
}

/**
 * @brief Scans the scratch buffer for every occurrence of one signature.
 *
 * Re-runs regexec from the byte just after the previous match so that multiple hits inside the same buffer (e.g. two SQLi tokens in one
 * URL) are all reported. The scan stops cleanly when no more matches exist or an allocation fails.
 *
 * @param entry The compiled signature to apply.
 * @param scratch NUL-terminated input.
 * @param scratch_len Length of scratch (excluding terminator).
 * @param matches_tail_ptr In/out: address of the .next pointer of the current tail of the match list. Updated as new records are linked in.
 * @param hit_count_inout In/out: incremented for each appended hit.
 * @return TRUE on success (zero or more hits), FALSE on allocation failure.
 */
static boolean_e http_signatures_scan_one_signature(http_signature_entry_t *entry, const char *scratch, size_t scratch_len, http_attack_match_t ***matches_tail_ptr, uint32_t *hit_count_inout)
{
    boolean_e scan_succeeded;
    boolean_e scan_active;
    regmatch_t match_position;
    int regexec_ret;
    size_t cursor;
    size_t absolute_match_start;
    size_t absolute_match_end;
    size_t match_len;
    http_attack_match_t *new_match;

    scan_succeeded = TRUE;
    scan_active = TRUE;
    cursor = 0;

    while (scan_active == TRUE)
    {
        regexec_ret = regexec(&entry->compiled_pattern, scratch + cursor, 1, &match_position, 0);

        if (regexec_ret == REG_NOMATCH)
        {
            scan_active = FALSE;
        }
        else if (regexec_ret != 0)
        {
            fprintf(stderr, "[L7][sig] regexec error %d on signature '%s'\n", regexec_ret, entry->signature_id);
            scan_active = FALSE;
        }
        else
        {
            absolute_match_start = cursor + (size_t)match_position.rm_so;
            absolute_match_end = cursor + (size_t)match_position.rm_eo;
            match_len = absolute_match_end - absolute_match_start;

            new_match = http_signatures_make_match(entry, scratch, absolute_match_start, match_len);
            if (new_match == NULL)
            {
                scan_succeeded = FALSE;
                scan_active = FALSE;
                fprintf(stderr, "[L7][sig] allocation failed for signature '%s' match\n", entry->signature_id);
            }
            else
            {
                **matches_tail_ptr = new_match;
                *matches_tail_ptr = &new_match->next;
                (*hit_count_inout)++;

                // Advance past this match. Empty matches (rm_so==rm_eo) would
                // loop forever, so we always step at least one byte forward.
                if (match_position.rm_eo == match_position.rm_so)
                {
                    cursor = absolute_match_start + 1;
                }
                else
                {
                    cursor = absolute_match_end;
                }

                if (cursor >= scratch_len)
                {
                    scan_active = FALSE;
                }
            }
        }
    }

    return scan_succeeded;
}

/**
 * @brief Scans a reassembled HTTP buffer for all registered attack signatures.
 *
 * Performs a one-time auto-init of the registry on the first call so .Copies the input into a NUL-safe scratch buffer, then
 * applies every compiled signature in turn.
 *
 * @param buffer Reassembled HTTP bytes (need not be NUL-terminated).
 * @param buffer_len Number of valid bytes in buffer.
 * @param out_matches_head In/out list head pointer; new matches are appended.
 * @param out_match_count Out: number of matches appended.
 * @return HTTP_SIG_SUCCESS or an error code.
 */
http_sig_ret_e http_signatures_scan_buffer(const uint8_t *buffer, size_t buffer_len, http_attack_match_t **out_matches_head, uint32_t *out_match_count)
{
    http_sig_ret_e ret_val;
    char scratch[HTTP_SIG_SCRATCH_BUFFER_LEN];
    size_t scratch_len;
    size_t signature_idx;
    boolean_e scan_succeeded;
    boolean_e processing_active;
    http_attack_match_t **matches_tail_ptr;

    ret_val = HTTP_SIG_SUCCESS;
    scan_succeeded = TRUE;
    processing_active = TRUE;

    // bad params
    if (buffer == NULL || out_matches_head == NULL || out_match_count == NULL)
    {
        ret_val = HTTP_SIG_NULL_ARG;
        processing_active = FALSE;
    }

    // init regex patterns
    if (processing_active == TRUE && g_http_signatures_initialized == FALSE)
    {
        if (http_signatures_init() != HTTP_SIG_SUCCESS)
        {
            ret_val = HTTP_SIG_REGCOMP_FAILED;
            processing_active = FALSE;
        }
    }

    if (processing_active == TRUE)
    {
        *out_match_count = 0;
        scratch_len = http_signatures_copy_for_regex(buffer, buffer_len, scratch);

        // Walk to the end of any pre-existing matches so we append rather than overwrite.
        matches_tail_ptr = out_matches_head;
        while (*matches_tail_ptr != NULL)
        {
            matches_tail_ptr = &((*matches_tail_ptr)->next);
        }

        signature_idx = 0;
        // Apply every signature in the registry, stopping on allocation failure.
        while (signature_idx < g_http_signature_count && scan_succeeded == TRUE)
        {
            if (g_http_signature_registry[signature_idx].is_compiled == TRUE)
            {
                scan_succeeded = http_signatures_scan_one_signature(&g_http_signature_registry[signature_idx], scratch, scratch_len, &matches_tail_ptr, out_match_count);
            }
            signature_idx++;
        }

        if (scan_succeeded == FALSE)
        {
            ret_val = HTTP_SIG_ALLOC_FAILED;
        }
    }

    return ret_val;
}

/**
 * @brief Frees a linked list of http_attack_match_t records.
 *
 * @param matches_head Head of the list to free. Safe with NULL.
 */
void http_signatures_free_matches(http_attack_match_t *matches_head)
{
    http_attack_match_t *current_match;
    http_attack_match_t *next_match;

    current_match = matches_head;

    while (current_match != NULL)
    {
        next_match = current_match->next;
        free(current_match);
        current_match = next_match;
    }
}