#include "l3_handler.h"



proto_handler_return_codes_e handle_l3_packet(const uint8_t *data, uint32_t len, packet_info_t *info)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;
    l3_ret_data_t ret_data;
    mac_table_proc_packet_data_t mac_data;


    switch (info->ether_type)
    {
        case ETH_P_IP:
            ret_data = handle_ipv4_protocol(data, len);
            ret_val = ret_data.ret_val;
            break;

        case ETH_P_IPV6:
            ret_data = handle_ipv6_protocol(data, len);
            ret_val = ret_data.ret_val;
            break;

        default:
            ret_val = PROTO_HANDLER_UNSUPPORTED_PROTOCOL;
            break;
    }

    if( ret_val == PROTO_HANDLER_SUCCESS)
    {
        info->ip_info = ret_data.ip_info;
        info->packet_len -= ret_data.header_len;

        mac_data.total_length = info->packet_len;

        //for src mac
        memcpy(mac_data.mac_addr, info->src_mac, ETH_ALEN);
        mac_data.ip_addr = info->ip_info.src_ip;
        mac_table_process_packet(mac_table_g, mac_data);

        //for dest mac
        memcpy(mac_data.mac_addr, info->dst_mac, ETH_ALEN);
        mac_data.ip_addr = info->ip_info.dst_ip;
        mac_table_process_packet(mac_table_g, mac_data);
    }

    return ret_val;
}
