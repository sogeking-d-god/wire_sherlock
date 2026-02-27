#ifndef ETH_HANDLER_H
#define ETH_HANDLER_H

#include "common.h"
#include "../l3/l3_handler.h"
#include "print_packet.h"

#define EXTENDED_ETH_TYPE ETH_P_8021Q // (when packet has vlan)
#define EXTENDED_ETH_HEADER_LEN ETH_HLEN + 4
#define TCI_LEN 2 // VLAN Tag Control Information

proto_handler_return_codes_e handle_l2_packet(const uint8_t *data, packet_info_t *info);

#endif
