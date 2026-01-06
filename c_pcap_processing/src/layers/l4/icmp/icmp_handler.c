#include "icmp_handler.h"

proto_handler_return_codes_e handle_icmp_packet(const uint8_t *data, uint32_t len, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;

    if(len < ICMP_MINLEN)
    {
        ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }

    else
    {
        struct icmp *icmp_hdr = (struct icmp *)data;
        info->packet_len -= ICMP_MINLEN;
    }

    return ret_val;
}