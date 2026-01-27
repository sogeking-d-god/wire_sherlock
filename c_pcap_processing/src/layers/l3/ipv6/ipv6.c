#include "ipv6.h"

l3_ret_data_t handle_ipv6_protocol(const uint8_t *data, uint32_t len)
{
    l3_ret_data_t ret_struct;

    struct ip6_hdr *ip_header = (struct ip6_hdr *)data;

    ret_struct.ret_val = PROTO_HANDLER_SUCCESS;

    if (len < IPV6_HEADER_LEN)
    {
        ret_struct.ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }

    else
    {
        memcpy(ret_struct.ip_info.src_ip.v6, &ip_header->ip6_src, IPV6_BYTES);
        memcpy(ret_struct.ip_info.dst_ip.v6, &ip_header->ip6_dst, IPV6_BYTES);
        ret_struct.ip_info.ip_version = IP_VERSION_6;
        ret_struct.ip_info.ip_proto = ip_header->ip6_ctlun.ip6_un1.ip6_un1_nxt;
        ret_struct.header_len = IPV6_HEADER_LEN;

        ip_tree_insert(ipv6_tree_g, ret_struct.ip_info.src_ip.v6, len);
        ip_tree_insert(ipv6_tree_g, ret_struct.ip_info.dst_ip.v6, len);
    }

    return ret_struct;
}

