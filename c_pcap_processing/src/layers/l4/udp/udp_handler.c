#include "udp_handler.h"

proto_handler_return_codes_e handle_udp_packet(const uint8_t *data, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;
    struct udphdr *udp_header;
    uint16_t packet_len;

    info->offsets.payload_offset = info->offsets.l4_offset + UDP_HEADER_LEN;

    if(info->cap_info.wire_len < info->offsets.payload_offset)
    {
        info->offsets.payload_offset = 0;
        ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }
    else if(info->cap_info.caplen < info->offsets.payload_offset)
    {
        info->offsets.payload_offset = 0;
        ret_val = PROTO_HANDLER_NOT_RECORDED_PACKET;
    }
    else
    {
        udp_header = (struct udphdr *)data;
        packet_len = ntohs(udp_header->uh_ulen);
        // because of ethernet padding up to 64, the length in the udp header needs to be less than the payload (and wire) len
        if(packet_len > info->offsets.payload_len)
        {
            info->offsets.payload_offset = 0;
            ret_val = PROTO_HANDLER_CORRUPT_PACKET;
        }
        else
        {
            info->port_info.src_port = ntohs(udp_header->uh_sport);
            info->port_info.dst_port = ntohs(udp_header->uh_dport);
            info->offsets.payload_len = packet_len;
        }
    }

    return ret_val;
}