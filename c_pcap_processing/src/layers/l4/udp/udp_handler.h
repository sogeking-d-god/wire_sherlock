#ifndef UDP_HANDLER_H
#define UDP_HANDLER_H

#include "common.h"


#define UDP_HEADER_LEN 8

proto_handler_return_codes_e handle_udp_packet(const uint8_t *data, uint32_t len, packet_info_t *info);

#endif