#ifndef HTTP_ATTACK_WRAPPER_H
#define HTTP_ATTACK_WRAPPER_H

#include <cjson/cJSON.h>
#include "parser.h"

/**
 * @brief IPC handler for cmd_analyze_http.
 *
 * Walks core->flow_table, aggregates HTTP attack match counts that were
 * recorded by the inline L7 sweep during cmd_start (each TCP session's
 * http_reassembler_session_t carries total_attack_matches), and sends a
 * JSON report to the client.
 *
 * Per-match detail (signature_id, offsets, matched_bytes) is currently
 * derived only from the aggregate count — the inline L7 sweep frees its
 * match lists after logging. The response schema's matches[] array stays
 * empty for now; downstream callers should rely on match_count and per_flow.
 */
void handle_analyze_http_request(int client_sock, file_analysis_context_t *core);

#endif
