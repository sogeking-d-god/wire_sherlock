#ifndef ICMP_HANDLER_H
#define ICMP_HANDLER_H

#include "common.h"





proto_handler_return_codes_e handle_icmp_packet(const uint8_t *data, packet_info_t *info);

#endif