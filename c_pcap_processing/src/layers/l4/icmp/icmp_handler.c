#include "icmp_handler.h"

proto_handler_return_codes_e handle_icmp_packet(const uint8_t *data, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;
    info->offsets.payload_offset = info->offsets.l4_offset + ICMP_MINLEN;

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
        struct icmp *icmp_hdr = (struct icmp *)data;
    }

    return ret_val;
}