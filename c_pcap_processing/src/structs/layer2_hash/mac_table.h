#ifndef MAC_TABLE_H
#define MAC_TABLE_H

#include "common.h"

#define MAC_ADDR_SIZE 18
#define MAC_HASH_SIZE 1024
#define MAC_HASH_CONST 5381
#define DJB2_SHIFT 5

#define MAC_NODE_SIZE sizeof(mac_node_t)
#define MAC_TABLE_SIZE sizeof(mac_table_t)


typedef struct mac_node
{
    uint8_t mac_addr[ETH_ALEN];

    struct mac_node *next;

    uint32_t packet_count;
    uint64_t total_bytes;

    ip_tree_t *ipv4_tree;
    ip_tree_t *ipv6_tree;
    // messages_linked_list_t sent_messages;
} mac_node_t;


typedef struct mac_table
{
    mac_node_t *buckets[MAC_HASH_SIZE];
    uint32_t total_devices;
} mac_table_t;

typedef enum
{
    MAC_TABLE_PROCESS_PACKET_INIT_IP_TREE_ERROR = -2,
    MAC_TABLE_PROCESS_PACKET_MALLOC_MAC_NODE_ERROR = -1,
    MAC_TABLE_PROCESS_PACKET_SUCCESS = 0,
    MAC_TABLE_PROCESS_PACKET_ADDED_NEW_NODE_SUCCESS = 1,
    MAC_TABLE_PROCESS_PACKET_ADD_TO_EXISTING_MAC_SUCCESS = 2,
} mac_table_process_packet_ret_code_e;

typedef struct
{
    mac_node_t * node;
    mac_table_process_packet_ret_code_e ret_code;
}mac_table_proc_packet_ret_t;


typedef struct
{
    uint8_t mac_addr [ETH_ALEN];
    uint64_t total_length;
} mac_table_proc_packet_data_t;

uint32_t mac_table_calculate_hash(const uint8_t *mac);

mac_table_t* mac_table_init();

void mac_table_print_report(mac_table_t *table);

void mac_table_free_table(mac_table_t *table);

mac_table_proc_packet_ret_t mac_table_process_packet(mac_table_t *table, const mac_table_proc_packet_data_t data);

typedef void (*mac_node_callback_fn)(mac_node_t *node, void *context);
void mac_table_iterate(mac_table_t *table, mac_node_callback_fn callback, void *context);

#endif