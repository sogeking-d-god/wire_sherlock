#ifndef IPV6H
#define IPV6H

#include "common.h"
#include "l4_handler.h"

#define IPV6_HEADER_LEN 40

proto_handler_return_codes_e handle_ipv6_protocol(const uint8_t *data, uint32_t len, packet_info_t *info);

#endif