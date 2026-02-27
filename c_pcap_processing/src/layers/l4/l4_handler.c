#include "l4_handler.h"


proto_handler_return_codes_e handle_l4_packet(const uint8_t *data, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;

    switch (info->ip_info.ip_proto)
    {
        case IPPROTO_TCP:
            ret_val = handle_tcp_packet(data, info);
            break;

        case IPPROTO_UDP:
            ret_val = handle_udp_packet(data, info);
            break;

        case IPPROTO_ICMP:
            ret_val = handle_icmp_packet(data, info);
            break;

        default:
            ret_val = PROTO_HANDLER_UNSUPPORTED_PROTOCOL;
            break;
    }


    return ret_val;
}

