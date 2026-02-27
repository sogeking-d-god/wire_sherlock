
#include "ipv4.h"

l3_ret_data_t handle_ipv4_protocol(const uint8_t *data, uint32_t len_left_recorded, uint32_t len_left_on_wire)
{
    // printf("DEBUG L3: First byte: 0x%02x, Expected: 0x45\n", *data);
    l3_ret_data_t ret_struct = {0};

    struct iphdr *ip_header = (struct iphdr *)data;

    if (len_left_on_wire < IPV4_HEADER_MIN_LEN )
    {
        ret_struct.ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }
    else if(len_left_recorded < IPV4_HEADER_MIN_LEN)
    {
        ret_struct.ret_val = PROTO_HANDLER_NOT_RECORDED_PACKET;
    }
    else
    {
        ret_struct.header_len = ip_header->ihl * IPV4_IHL_TO_BYTES;
        ret_struct.packet_len = ntohs(ip_header->tot_len);
        ret_struct.ret_val = PROTO_HANDLER_SUCCESS;
        printf("DEBUG L3 Validation: TotLen: %u, HeaderLen: %u, OnWire: %u\n",
            ret_struct.packet_len, ret_struct.header_len, len_left_on_wire);

        // TSO
        if (ret_struct.packet_len == 0)
        {
            printf("DEBUG: entered tso, packet_len: %d!\n",  len_left_on_wire);
            ret_struct.packet_len = len_left_on_wire;
        }

        if (ret_struct.packet_len > len_left_on_wire || ret_struct.header_len > ret_struct.packet_len ||
            ret_struct.header_len > IPV4_MAX_HEADER_LEN)
        {
            printf("DEBUG: entered PROTO_HANDLER_CORRUPT_PACKET\n");
            printf("DEBUG FAIL: IP:%u.%u.%u.%u | TotLen:%u | HdrLen:%u | OnWire:%u | RawIHL:%u\n",
        data[12], data[13], data[14], data[15],
        ret_struct.packet_len, ret_struct.header_len, len_left_on_wire, ip_header->ihl);
        printf("\n\n");
            ret_struct.ret_val = PROTO_HANDLER_CORRUPT_PACKET;
        }
        else
        {
            printf("DEBUG L3: Inserting to tree: %u.%u.%u.%u\n", data[12], data[13], data[14], data[15]);

            memcpy(ret_struct.ip_info.src_ip.v4, &ip_header->saddr, IPV4_BYTES);
            memcpy(ret_struct.ip_info.dst_ip.v4, &ip_header->daddr, IPV4_BYTES);

            ret_struct.ip_info.ip_version = IP_VERSION_4;
            ret_struct.ip_info.ip_proto = ip_header->protocol;
            ret_struct.packet_len -= ret_struct.header_len;

            ip_tree_insert(ipv4_tree_g, ret_struct.ip_info.src_ip.v4, len_left_on_wire);
            ip_tree_insert(ipv4_tree_g, ret_struct.ip_info.dst_ip.v4, len_left_on_wire);
        }
    }
    return ret_struct;
}
