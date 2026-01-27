#ifndef IP_TREE_H
#define IP_TREE_H

#include "common.h"

#define IP_TREE_NODE_SIZE sizeof(ip_tree_node_t)
#define IP_TREE_TREE_SIZE sizeof(ip_tree_t)

#define IP_TREE_BIT_MASK_START_VAL 1<<7

// ipv6 has 128 bit, 128 is 2 ^ 7
#define IP_TREE_IPV6_BITS_COUNT_IN_BITS 7


typedef struct ip_stats
{
    uint32_t total_packets;
    uint64_t total_bytes;
} ip_tree_stats_t;


typedef struct ip_tree_node
{
    ip_tree_stats_t stats;
    uint8_t * ip;
    struct ip_tree_node *children[2];
    uint8_t diff_bit_index : IP_TREE_IPV6_BITS_COUNT_IN_BITS;
} ip_tree_node_t;


typedef struct ip_tree
{
    ip_tree_node_t *root;
    uint32_t total_unique_ips;
    // amount of bytes in ip addr -1
    uint8_t ip_addr_byte_count;
} ip_tree_t;

typedef enum
{
    IP_TREE_RET_SUCCESS = 0,
    IP_TREE_RET_MALLOC_FAIL_ERROR = -1,
}ip_tree_ret_codes_e;

typedef struct
{
    uint8_t bit_val : 1;
    uint8_t bit_index : IP_TREE_IPV6_BITS_COUNT_IN_BITS;
    boolean_e bit_found_flag;
}ip_tree_diff_bit_ret_t;

typedef struct
{
    uint8_t bytes : 4;
    uint8_t bits : 3;
}ip_tree_diff_bit_bit_range_t;




ip_tree_ret_codes_e ip_tree_insert(ip_tree_t *tree, uint8_t * ip, uint32_t packet_len);
ip_tree_t* ip_tree_init(uint8_t ip_addr_byte_count);
void ip_tree_free_tree(ip_tree_t* tree);
void ip_tree_print_report(ip_tree_t *tree);

#endif