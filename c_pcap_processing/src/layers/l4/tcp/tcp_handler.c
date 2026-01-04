#include "tcp_handler.h"

proto_handler_return_codes_e handle_tcp_packet(const uint8_t *data, uint32_t len, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;

    if(len < TCP_HEADER_MIN_LEN)
    {
        ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }

    else
    {
        struct tcphdr *tcp_header = (struct tcphdr *)data;

        info->src_port = ntohs(tcp_header->th_sport);
        info->dst_port = ntohs(tcp_header->th_dport);

        uint8_t data_offset = tcp_header->th_off * TCP_OFFSET_IN_BYTES;

        if(len < data_offset)
        {
            ret_val = PROTO_HANDLER_CORRUPT_PACKET;
        }

        else
        {
            info->packet_len = len - data_offset;

        }
    }

    return ret_val;
}