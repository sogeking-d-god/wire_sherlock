#ifndef FLOW_TABLE_H
#define FLOW_TABLE_H

#include "common.h"
#include "parser.h"

#define FLOW_HASH_SIZE 2048
#define FLOW_HASH_CONST 0x12345678
#define DEVICES_IN_FLOW 2
#define PORT_BIT_COUNT 16

#define FLOW_TABLE_TIMEOUT 60

#define MESSAGE_NODE_SIZE sizeof(message_node_t)
#define SESSION_NODE_SIZE sizeof(session_node_t)
#define FLOW_KEY_SIZE sizeof(flow_key_t)
#define FLOW_NODE_SIZE sizeof(flow_node_t)
#define FLOW_TABLE_SIZE sizeof(flow_table_t)

typedef enum
{
    FLOW_TABLE_FIRST_DEVICE_SRC = 0,
    FLOW_TABLE_FIRST_DEVICE_DST = 1
} flow_table_first_device_e;

typedef enum
{
    FLOW_TABLE_PROCESS_PACKET_INVALID_PARAMS_ERROR = -3,
    FLOW_TABLE_PROCESS_PACKET_MALLOC_SESSION_ERROR = -2,
    FLOW_TABLE_PROCESS_PACKET_MALLOC_FLOW_NODE_ERROR = -1,
    FLOW_TABLE_PROCESS_PACKET_SUCCESS = 0,
    FLOW_TABLE_PROCESS_PACKET_ADDED_NEW_NODE_SUCCESS = 1,
    FLOW_TABLE_PROCESS_PACKET_MALLOC_MESSAGE_NODE_ERROR = 2,
    FLOW_TABLE_PROCESS_PACKET_ADD_TO_EXISTING_FLOW_SUCCESS = 3,
    FLOW_TABLE_PROCESS_PACKET_ADDED_NEW_SESSION_SUCCESS = 4,
} flow_table_process_packet_return_e;

typedef enum
{
    FLOW_TABLE_TCP_START_STATE_IDLE, // the handshake wasnt detected...
    FLOW_TABLE_TCP_START_STATE_SYN_SENT,
    FLOW_TABLE_TCP_START_STATE_SYN_ACK_SENT,
    FLOW_TABLE_TCP_START_STATE_HANDSHAKE_COMPLETE,
}flow_table_tcp_start_state_e;
typedef enum
{
    FLOW_TABLE_TCP_END_STATE_NOT_CLOSED = -3,
    FLOW_TABLE_TCP_END_STATE_FIN_SENT_BY_SRC = -2,
    FLOW_TABLE_TCP_END_STATE_FIN_SENT_BY_DEST = -1,
    FLOW_TABLE_TCP_END_STATE_CLOSED_GRACEFULLY = 1,
    FLOW_TABLE_TCP_END_STATE_CLOSED_UNGRACEFULLY = 2,
} flow_table_tcp_end_state_e;

// Linked list node for a single message within a flow
typedef struct message_node
{
    uint32_t seq_num;
    struct timeval timestamp;

    uint32_t payload_len; // Length of the payload (without the headers for easy reading from file)
    uint32_t data_ptr; // pointer to NEW data in the message (excledes already processed seqs)
    uint32_t total_packet_len; // Total length of the packet including headers
    uint32_t packet_start_pointer;// Pointer to the start of the packet in the pcap file
    uint8_t tcp_flags;

    struct message_node *next;
} message_node_t;

// Linked list to hold messages for a flow
typedef struct
{
    message_node_t *head;
    message_node_t *tail;
} messages_linked_list_t;

// Structure to hold device-specific data within a flow
typedef struct
{
    uint32_t packets_sent;
    uint32_t bytes_sent;

    uint32_t last_seq;
    uint32_t last_ack;
    uint32_t next_expected_seq;
} flow_device_data_t;

// Structure that holds device information in a flow
typedef struct
{
    flow_device_data_t data;

    message_node_t *ooo_buffer; // out-of-order
} flow_device_t;


// Key structure to identify a flow
typedef struct
{
    ip_addr_t src_ip;
    ip_addr_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  protocol;
    ip_version_e ip_type;
} flow_key_t;

typedef struct session_node
{
    struct timeval timestamp;
    flow_table_tcp_start_state_e start_state;
    flow_table_tcp_end_state_e end_state;

    flow_device_t devices[DEVICES_IN_FLOW]; // Device-specific data for both endpoints, (ordered by increasing IP)

    messages_linked_list_t messages;

    // Pointer to the next flow in the bucket (for collision handling)
    struct session_node *next;
} session_node_t;


// Node structure for each flow in the hash table
typedef struct flow_node
{
    flow_key_t key;

    session_node_t * first_session;
    session_node_t * last_session;

    // Pointer to the next flow in the bucket (for collision handling)
    struct flow_node *next;
} flow_node_t;

// Hash table structure of flows
typedef struct flow_table
{
    flow_node_t *buckets[FLOW_HASH_SIZE];
    uint32_t flow_count;
} flow_table_t;


typedef struct
{
    flow_table_process_packet_return_e ret_code;
    flow_node_t * flow_node_ptr;
    flow_table_first_device_e dev_idx;
}flow_table_process_packet_ret_t;


flow_table_t* flow_table_init();
flow_table_process_packet_ret_t flow_table_process_packet(flow_table_t *table, packet_info_t *info);
void flow_table_print_report(flow_table_t *table);
void flow_table_free_table(flow_table_t *table);
void flow_table_free_node(flow_node_t *node);
char* get_ip_str(const ip_addr_t *ip, ip_version_e ver, char * buf, size_t buflen);
const char* get_tcp_state_str(session_node_t *sess, uint8_t protocol);
flow_table_process_packet_return_e flow_table_insert_to_session(flow_node_t * flow_node, packet_info_t * info, flow_table_first_device_e src_dev);


typedef void (*flow_callback_fn)(flow_node_t *node, void *context);

void flow_table_iterate(flow_table_t *table, flow_callback_fn callback, void *context);

#endif