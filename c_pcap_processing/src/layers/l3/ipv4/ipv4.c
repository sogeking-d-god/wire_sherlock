
#include "ipv4.h"

l3_ret_data_t handle_ipv4_protocol(const uint8_t *data, uint32_t len)
{

    l3_ret_data_t ret_struct;

    struct iphdr *ip_header = (struct iphdr *)data;
    ret_struct.header_len = ip_header->ihl * IPV4_IHL_TO_BYTES;
    ret_struct.ret_val = PROTO_HANDLER_SUCCESS;

    if (len < IPV4_HEADER_MIN_LEN || ret_struct.header_len > len)
    {
        ret_struct.ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }

    else
    {
        memcpy(ret_struct.ip_info.src_ip.v4, &ip_header->saddr, IPV4_BYTES);
        memcpy(ret_struct.ip_info.dst_ip.v4, &ip_header->daddr, IPV4_BYTES);

        ret_struct.ip_info.ip_version = IP_VERSION_4;
        ret_struct.ip_info.ip_proto = ip_header->protocol;

        ip_tree_insert(ipv4_tree_g, ret_struct.ip_info.src_ip.v4, len);
        ip_tree_insert(ipv4_tree_g, ret_struct.ip_info.dst_ip.v4, len);


    }
    return ret_struct;
}

