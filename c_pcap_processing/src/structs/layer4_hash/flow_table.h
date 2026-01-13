#ifndef FLOW_TABLE_H
#define FLOW_TABLE_H

#include "common.h"
#include "parser.h"

#define FLOW_HASH_SIZE 2048
#define FLOW_HASH_CONST 0x12345678
#define DEVICES_IN_FLOW 2

#define KEY_SIZE sizeof(flow_key_t)
#define FLOW_NODE_SIZE sizeof(flow_node_t)
#define MESSAGE_NODE_SIZE sizeof(message_node_t)
#define FLOW_TABLE_SIZE sizeof(flow_table_t)


// Linked list node for a single message within a flow
typedef struct message_node
{
    uint32_t seq_num;
    uint64_t timestamp;

    uint32_t payload_len; // Length of the payload (without the headers for easy reading from file)
    uint32_t total_packet_len; // Total length of the packet including headers
    uint32_t packet_start_pointer;// Pointer to the start of the packet in the pcap file

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
} flow_device_data_t;

// Structure that holds device information in a flow
typedef struct
{
    uint32_t ip;
    uint16_t port;
    flow_device_data_t data;
} flow_device_t;


// Key structure to identify a flow
typedef struct
{
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  protocol;
} flow_key_t;

// Node structure for each flow in the hash table
typedef struct flow_node
{
    flow_key_t key;

    uint8_t protocol;
    flow_device_t devices[DEVICES_IN_FLOW]; // Device-specific data for both endpoints, (ordered by increasing IP)

    messages_linked_list_t messages;

    // Pointer to the next flow in the bucket (for collision handling)
    struct flow_node *next;
} flow_node_t;

// Hash table structure of flows
typedef struct flow_table
{
    flow_node_t *buckets[FLOW_HASH_SIZE];
    uint32_t flow_count;
} flow_table_t;

typedef enum
{
    FLOW_TABLE_FIRST_DEVICE_SRC = 0,
    FLOW_TABLE_FIRST_DEVICE_DST = 1
} flow_table_first_device_e;

typedef enum
{
    FLOW_TABLE_PROCESS_PACKET_INVALID_PARAMS = -2,
    FLOW_TABLE_PROCESS_PACKET_MALLOC_FLOW_NODE_ERROR = -1,
    FLOW_TABLE_PROCESS_PACKET_SUCCESS = 0,
    FLOW_TABLE_PROCESS_PACKET_ADDED_NEW_NODE_SUCCESS = 1,
    FLOW_TABLE_PROCESS_PACKET_MALLOC_MESSAGE_NODE_ERROR = 2,
    FLOW_TABLE_PROCESS_PACKET_ADD_TO_EXISTING_FLOW_SUCCESS = 3,
} flow_table_process_packet_return_e;

flow_table_t* flow_table_init();
flow_table_process_packet_return_e flow_table_process_packet(flow_table_t *table, packet_info_t *info);
void flow_table_print_report(flow_table_t *table);
void flow_table_free_table(flow_table_t *table);
void flow_table_free_node(flow_node_t *node);

#endif