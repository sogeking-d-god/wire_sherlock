#ifndef ETH_HANDLER_H
#define ETH_HANDLER_H

#include "common.h"
#include "../l3/l3_handler.h"
#include "print_packet.h"

proto_handler_return_codes_e handle_l2_packet(const uint8_t *data, uint32_t len, packet_info_t *info) ;

#endif
