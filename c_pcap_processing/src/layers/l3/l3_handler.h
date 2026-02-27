#ifndef L3_MANAGER_H
#define L3_MANAGER_H


#include "../../common.h"
#include "./ipv4/ipv4.h"
#include "./ipv6/ipv6.h"







proto_handler_return_codes_e handle_l3_packet(const uint8_t *data, packet_info_t *info);

#endif