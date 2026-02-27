#include "eth_handler.h"

//TODO add mac address hash table with data like : {ip arr, packet count ...}

proto_handler_return_codes_e handle_l2_packet(const uint8_t *data, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;

    struct ethhdr *eth = (struct ethhdr *)data;
    uint16_t ether_type = ntohs(eth->h_proto);

    // check if the length is at least the size of Ethernet header
    if (info->cap_info.wire_len < ETH_HLEN)
    {
        ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }
    else if (info->cap_info.caplen < ETH_HLEN)
    {
        ret_val = PROTO_HANDLER_NOT_RECORDED_PACKET;
    }
    //call next layer handler based on ether_type
    else
    {
        memcpy(info->mac_info.dst_mac, eth->h_dest, ETH_ALEN);
        memcpy(info->mac_info.src_mac, eth->h_source, ETH_ALEN);

        if(ether_type == EXTENDED_ETH_TYPE)
        {
            info->mac_info.tci = ntohs(*(uint16_t *)(data + ETH_HLEN));
            info->mac_info.ether_type = ntohs(*(uint16_t *)(data + EXTENDED_ETH_HEADER_LEN - TCI_LEN));
            info->offsets.l3_offset = EXTENDED_ETH_HEADER_LEN;
        }
        else
        {
            info->mac_info.tci = 0;
            info->mac_info.ether_type = ether_type;
            info->offsets.l3_offset = ETH_HLEN;
        }
    }
    return ret_val;
}