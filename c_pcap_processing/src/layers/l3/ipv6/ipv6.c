#include "ipv6.h"
#include <netinet/ip6.h>

l3_ret_data_t handle_ipv6_protocol(const uint8_t *data, uint32_t len_left_recorded, uint32_t len_left_on_wire, ip_tree_t *ipv6_tree, uint32_t len_for_stats)
{
    l3_ret_data_t ret_struct;
    struct ip6_hdr *ip_header;
    struct ip6_ext *ext_ptr;
    uint8_t current_nxt;
    uint32_t offset;
    uint32_t ext_len;
    const uint8_t *current_pos;
    uint8_t keep_looping;

    ret_struct.ret_val = PROTO_HANDLER_SUCCESS;
    ret_struct.header_len = sizeof(struct ip6_hdr);
    ip_header = (struct ip6_hdr *)data;
    offset = sizeof(struct ip6_hdr);
    keep_looping = 1;

    if (len_left_on_wire < sizeof(struct ip6_hdr))
    {
        ret_struct.ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }
    else if (len_left_recorded < sizeof(struct ip6_hdr))
    {
        ret_struct.ret_val = PROTO_HANDLER_NOT_RECORDED_PACKET;
    }
    else
    {
        ret_struct.ip_info.ip_version = IP_VERSION_6;
        ret_struct.packet_len = ntohs(ip_header->ip6_plen);
        current_nxt = ip_header->ip6_nxt;

        if (ret_struct.packet_len == 0)
        {
            ret_struct.packet_len = len_left_on_wire - sizeof(struct ip6_hdr);
        }

        if (ret_struct.packet_len + sizeof(struct ip6_hdr) > len_left_on_wire)
        {
            ret_struct.ret_val = PROTO_HANDLER_CORRUPT_PACKET;
        }
        else
        {
            memcpy(ret_struct.ip_info.src_ip.v6, &ip_header->ip6_src, IPV6_BYTES);
            memcpy(ret_struct.ip_info.dst_ip.v6, &ip_header->ip6_dst, IPV6_BYTES);

            ip_tree_insert(ipv6_tree, ret_struct.ip_info.src_ip.v6, len_for_stats);
            ip_tree_insert(ipv6_tree, ret_struct.ip_info.dst_ip.v6, len_for_stats);

            // Process extension headers if they exist, and find the actual L4 protocol
            while (keep_looping == 1 && (current_nxt == IPPROTO_HOPOPTS || current_nxt == IPPROTO_ROUTING ||
                   current_nxt == IPPROTO_FRAGMENT || current_nxt == IPPROTO_DSTOPTS))
            {
                if (offset + sizeof(struct ip6_ext) > len_left_on_wire)
                {
                    ret_struct.ret_val = PROTO_HANDLER_CORRUPT_PACKET;
                    keep_looping = 0;
                }
                else
                {
                    current_pos = data + offset;
                    ext_ptr = (struct ip6_ext *)current_pos;

                    if (current_nxt == IPPROTO_FRAGMENT)
                    {
                        ext_len = sizeof(struct ip6_frag);
                    }
                    else
                    {
                        ext_len = (ext_ptr->ip6e_len + 1) * OCTET_SIZE;
                    }

                    if (ext_len == 0 || (offset + ext_len) > len_left_on_wire)
                    {
                        ret_struct.ret_val = PROTO_HANDLER_CORRUPT_PACKET;
                        keep_looping = 0;
                    }
                    else
                    {
                        current_nxt = ext_ptr->ip6e_nxt;
                        offset += ext_len;

                        if (ret_struct.packet_len >= ext_len)
                        {
                            ret_struct.packet_len -= ext_len;
                        }
                        else
                        {
                            ret_struct.packet_len = 0;
                        }
                    }
                }
            }
            ret_struct.ip_info.ip_proto = current_nxt;
            ret_struct.header_len = offset;
        }
    }
    return ret_struct;
}