
#include "ipv4.h"

proto_handler_return_codes_e handle_ipv4_protocol(const uint8_t *data, uint32_t len, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;

    struct iphdr *ip_header = (struct iphdr *)data;
    uint32_t ip_header_len = ip_header->ihl * 4;

    if (len < IPV4_HEADER_MIN_LEN || ip_header_len > len)
    {
        ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }

    else
    {
        memcpy(info->src_ip.v4, &ip_header->saddr, IPV4_BYTES);
        memcpy(info->dst_ip.v4, &ip_header->daddr, IPV4_BYTES);
        info->ip_version = IP_VERSION_4;
        info->ip_proto = ip_header->protocol;

    }

    if (ret_val == PROTO_HANDLER_SUCCESS)
    {
        ret_val = handle_l4_packet(data + ip_header_len, len - ip_header_len, info);
    }

    return ret_val;
}

