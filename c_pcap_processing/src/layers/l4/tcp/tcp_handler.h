#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H

#include "common.h"


#define TCP_HEADER_MIN_LEN 20
#define TCP_OFFSET_IN_BYTES 4

proto_handler_return_codes_e handle_tcp_packet(const uint8_t *data, uint32_t len, packet_info_t *info);

#endif