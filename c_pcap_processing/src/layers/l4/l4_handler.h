#ifndef L4_HANDLER_H
#define L4_HANDLER_H

#include "common.h"

#include "tcp_handler.h"
#include "udp_handler.h"
#include "icmp_handler.h"

proto_handler_return_codes_e handle_l4_packet(const uint8_t *data, uint32_t len, packet_info_t *info);

#endif