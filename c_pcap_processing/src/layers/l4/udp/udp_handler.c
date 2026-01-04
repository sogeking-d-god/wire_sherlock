#include "udp_handler.h"

proto_handler_return_codes_e handle_udp_packet(const uint8_t *data, uint32_t len, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;

    if(len < UDP_HEADER_LEN)
    {
        ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }

    else
    {
        struct udphdr *udp_header = (struct udphdr *)data;

        info->src_port = ntohs(udp_header->uh_sport);
        info->dst_port = ntohs(udp_header->uh_dport);
        info->packet_len = ntohs(udp_header->uh_ulen) - UDP_HEADER_LEN;
    }

    return ret_val;
}