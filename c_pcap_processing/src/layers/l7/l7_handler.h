#ifndef L7_HANDLER_H
#define L7_HANDLER_H

#include "common.h"

typedef enum {
    L7_PROTO_UNKNOWN = 0,
    L7_PROTO_HTTP1,
    L7_PROTO_TLS,
    L7_PROTO_HTTP2,
    L7_PROTO_NOT_TEXT
} l7_protocol_e;

typedef struct http_message_record {
    uint8_t dev_idx;
    uint32_t start_msg_seq;
    uint32_t header_byte_len;
    struct http_message_record *next;
} http_message_record_t;

typedef struct l7_session_state_s {
    l7_protocol_e proto;
    http_message_record_t *messages_dev0;
    http_message_record_t *messages_dev1;
    uint32_t attack_count;
} l7_session_state_t;

typedef enum {
    L7_HANDLER_SUCCESS = 0,
    L7_HANDLER_NULL_ARG = -1,
    L7_HANDLER_PCAP_OPEN_FAILED = -2
} l7_handler_ret_e;

l7_handler_ret_e l7_handler_run(flow_table_t *table, const char *pcap_path);
void l7_session_state_free(l7_session_state_t *state);

#endif
