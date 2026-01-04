#ifndef ICMP_HANDLER_H
#define ICMP_HANDLER_H

#include "common.h"



proto_handler_return_codes_e handle_icmp_packet(const uint8_t *data, uint32_t len, packet_info_t *info);

#endif