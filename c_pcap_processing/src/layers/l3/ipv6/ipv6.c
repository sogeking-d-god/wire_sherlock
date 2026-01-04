#include "ipv6.h"

proto_handler_return_codes_e handle_ipv6_protocol(const uint8_t *data, uint32_t len, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;

    struct ip6_hdr *ip_header = (struct ip6_hdr *)data;


    if (len < IPV6_HEADER_LEN)
    {
        ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }

    else
    {
        memcpy(info->src_ip.v6, &ip_header->ip6_src, IPV6_BYTES);
        memcpy(info->dst_ip.v6, &ip_header->ip6_dst, IPV6_BYTES);
        info->ip_version = IP_VERSION_6;
        info->ip_proto = ip_header->ip6_ctlun.ip6_un1.ip6_un1_nxt;

    }

    if (ret_val == PROTO_HANDLER_SUCCESS)
    {
        handle_l4_packet(data + IPV6_HEADER_LEN, len - IPV6_HEADER_LEN, info);
    }

    return ret_val;
}

