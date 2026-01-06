#ifndef IPV4_H
#define IPV4_H

#include "common.h"
#include "l4_handler.h"

#define IPV4_HEADER_MIN_LEN 20

proto_handler_return_codes_e handle_ipv4_protocol(const uint8_t *data, uint32_t len, packet_info_t *info);

#endif