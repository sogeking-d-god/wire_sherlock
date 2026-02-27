#ifndef COMMON_H
#define COMMON_H


#define __USE_MISC 1 // for tcp.h
#define _DEFAULT_SOURCE 1



#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>



#define IPV4_BYTES 4
#define IPV6_BYTES 16
#define BITS_IN_BYTE 8

typedef enum {
    IP_VERSION_4 = 4,
    IP_VERSION_6 = 6
} ip_version_e;

typedef enum {
    FALSE = 0,
    TRUE  = 1
} boolean_e;

typedef union
{
    uint8_t v4[IPV4_BYTES];
    uint8_t  v6[IPV6_BYTES];
} ip_addr_t;

typedef struct
{
    ip_addr_t ip_addr;
    ip_version_e ip_version;
}ip_and_type_t;



typedef struct
{
    ip_addr_t src_ip;
    ip_addr_t dst_ip;
    ip_version_e ip_version;
    uint8_t  ip_proto;
}ip_info_t;

//TO-DO add more data and stat fields for flow table processing

typedef struct
{
    uint16_t l3_offset;
    uint16_t l4_offset;
    uint16_t payload_offset; // data start
    uint16_t payload_len;
    uint32_t packet_start_pointer; // packet start IN FILE ptr
}packet_info_offsets_t;

typedef struct
{
    struct timeval ts; // timestamp
    uint32_t caplen; // length that was actually saved
    uint32_t wire_len; // original packet length
}packet_cap_info_t;

typedef struct
{
    uint8_t src_mac[ETH_ALEN];
    uint8_t dst_mac[ETH_ALEN];
    uint16_t ether_type;
    uint8_t tci; //VLAN Tag Control Information: {PCP (Priority Code Point), DEI (Drop Eligible Indicator), VID (VLAN Identifier)}
}packet_mac_info_t;

typedef struct
{
    uint16_t src_port;
    uint16_t dst_port;

    uint8_t tcp_flags;
}packet_port_info_t;

typedef struct
{
    // Capture info
    packet_cap_info_t cap_info;

    // Layer 2 (Ethernet)
    packet_mac_info_t mac_info;

    // Layer 3 (IP)
    ip_info_t ip_info;

    // Layer 4 (TCP/UDP)
    packet_port_info_t port_info;

    packet_info_offsets_t offsets;
} packet_info_t;

typedef enum {
    PROTO_HANDLER_SUCCESS = 0,
    PROTO_HANDLER_UNSUPPORTED_PROTOCOL = -1,
    PROTO_HANDLER_CORRUPT_PACKET = -2,
    PROTO_HANDLER_NOT_RECORDED_PACKET = -3,
} proto_handler_return_codes_e;

typedef struct
{
    proto_handler_return_codes_e ret_val;
    uint32_t header_len;
    ip_info_t ip_info;
    uint16_t packet_len; //payload length from ip header (not including header)
}l3_ret_data_t;

/**
 * @brief A function pointer type for protocol handlers
 *
 * @param data Pointer to the packet data (of the next layer)
 * @param len Length of the packet data that was sniffed that is left to process
 * @param info Pointer to the packet information structure
 *
 * @return Returns a proto_handler_return_codes_e indicating success or type of error
 */
typedef proto_handler_return_codes_e (*protocol_handler_t) (const uint8_t * data, uint32_t len, packet_info_t * info);

typedef struct mac_table mac_table_t;
typedef struct flow_table flow_table_t;
typedef struct ip_tree ip_tree_t;

#include "mac_table.h"
#include "flow_table.h"
#include "ip_tree.h"

extern mac_table_t * mac_table_g;
extern ip_tree_t * ipv4_tree_g;
extern ip_tree_t * ipv6_tree_g;

#endif