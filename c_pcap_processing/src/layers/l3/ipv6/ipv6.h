#ifndef IPV6H
#define IPV6H

#include "common.h"
#include "l3_handler.h"

#define IPV6_HEADER_LEN 40
#define OCTET_SIZE 8

l3_ret_data_t handle_ipv6_protocol(const uint8_t *data, uint32_t len_left_recorded, uint32_t len_left_on_wire, ip_tree_t *ipv6_tree);

#endif