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

typedef enum {
    IP_VERSION_4 = 4,
    IP_VERSION_6 = 6
} ip_version_t;

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
    ip_addr_t src_ip;
    ip_addr_t dst_ip;
    ip_version_t ip_version;
    uint8_t  ip_proto;
}ip_info_t;

//TO-DO add more data and stat fields for flow table processing
typedef struct
{
    // Layer 2 (Ethernet)
    uint8_t src_mac[ETH_ALEN];
    uint8_t dst_mac[ETH_ALEN];
    uint16_t ether_type;

    // Layer 3 (IP)
    ip_info_t ip_info;

    // Layer 4 (TCP/UDP)
    uint16_t src_port;
    uint16_t dst_port;

    uint32_t packet_len;
} packet_info_t;

typedef enum {
    PROTO_HANDLER_SUCCESS = 0,
    PROTO_HANDLER_UNSUPPORTED_PROTOCOL = -1,
    PROTO_HANDLER_CORRUPT_PACKET = -2
} proto_handler_return_codes_e;

typedef struct
{
    proto_handler_return_codes_e ret_val;
    uint32_t header_len;
    ip_info_t ip_info;
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

#include "mac_table.h"
#include "flow_table.h"


extern mac_table_t * mac_table_g;

#endif