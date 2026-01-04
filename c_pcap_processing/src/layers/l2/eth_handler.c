#include "eth_handler.h"

//TODO add mac address hash table with data like : {ip arr, packet count ...}

proto_handler_return_codes_e handle_l2_packet(const uint8_t *data, uint32_t len, packet_info_t *info)
{

    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;
    proto_handler_return_codes_e next_layer_ret_val;

    struct ethhdr *eth = (struct ethhdr *)data;
    uint16_t ether_type = ntohs(eth->h_proto);

    // check if the length is at least the size of Ethernet header
    if (len < ETH_HLEN)
    {
        ret_val = PROTO_HANDLER_CORRUPT_PACKET;
    }

    //call next layer handler based on ether_type
    else
    {
        memcpy(info->dst_mac, eth->h_dest, ETH_ALEN);
        memcpy(info->src_mac, eth->h_source, ETH_ALEN);
        info->ether_type = ether_type;
        next_layer_ret_val = handle_l3_packet(data + ETH_HLEN, len - ETH_HLEN, info);
    }

    if(ret_val == PROTO_HANDLER_SUCCESS)
    {
        print_packet_summary(info);
    }

    return ret_val;
}