#ifndef IPV4_H
#define IPV4_H

#include "common.h"
#include "l3_handler.h"

#define IPV4_HEADER_MIN_LEN 20
#define IPV4_IHL_TO_BYTES 4
#define IPV4_MAX_HEADER_LEN 60

l3_ret_data_t handle_ipv4_protocol(const uint8_t *data, uint32_t len_left_recorded, uint32_t len_left_on_wire, ip_tree_t *ipv4_tree);

#endif