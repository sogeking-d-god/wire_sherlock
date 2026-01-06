#include "l3_handler.h"



proto_handler_return_codes_e handle_l3_packet(const uint8_t *data, uint32_t len, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;
    // proto_handler_return_codes_e next_layer_ret_val;

    switch (info->ether_type)
    {
        case ETH_P_IP:
            ret_val = handle_ipv4_protocol(data, len, info);
            break;

        case ETH_P_IPV6:
            ret_val = handle_ipv6_protocol(data, len, info);
            break;

        default:
            ret_val = PROTO_HANDLER_UNSUPPORTED_PROTOCOL;
            break;
    }

    return ret_val;
}
