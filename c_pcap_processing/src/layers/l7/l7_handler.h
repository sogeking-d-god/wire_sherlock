#ifndef L7_HANDLER_H
#define L7_HANDLER_H

#include "common.h"

typedef enum
{
    L7_PROTO_UNKNOWN = 0,
    L7_PROTO_HTTP1,
    L7_PROTO_TLS,
    L7_PROTO_HTTP2,
    L7_PROTO_NOT_TEXT
} l7_protocol_e;

typedef enum
{
    L7_HANDLER_SUCCESS = 0,
    L7_HANDLER_NULL_ARG = -1,
    L7_HANDLER_PCAP_OPEN_FAILED = -2,
    L7_HANDLER_SIG_INIT_FAILED = -3
} l7_handler_ret_e;

/**
 * @brief Runs the L7 sweep over every TCP session in the flow table.
 *
 * Opens the PCAP file, iterates each TCP session, feeds payload bytes
 * into the per-session HTTP reassembler, and scans each completed HTTP
 * message with the signature engine. Logs all hits to stderr.
 *
 * @param table The flow table to sweep.
 * @param pcap_path Path to the PCAP file for re-reading payload bytes.
 * @return L7_HANDLER_SUCCESS on success, error code on failure.
 */
l7_handler_ret_e l7_handler_run(flow_table_t *table, const char *pcap_path);

#endif
