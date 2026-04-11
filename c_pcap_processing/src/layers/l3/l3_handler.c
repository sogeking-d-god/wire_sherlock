#include "l3_handler.h"



proto_handler_return_codes_e handle_l3_packet(const uint8_t *data, packet_info_t *info, file_analysis_context_t *core)
{
    proto_handler_return_codes_e ret_val = PROTO_HANDLER_SUCCESS;
    l3_ret_data_t ret_data;
    uint32_t len_left_recorded = info->cap_info.caplen - info->offsets.l3_offset;
    uint32_t len_left_on_wire = info->cap_info.wire_len - info->offsets.l3_offset;


    switch (info->mac_info.ether_type)
    {
        case ETH_P_IP:
            ret_data = handle_ipv4_protocol(data, len_left_recorded, len_left_on_wire, core->ipv4_tree);
            ret_val = ret_data.ret_val;
            break;

        case ETH_P_IPV6:
            ret_data = handle_ipv6_protocol(data, len_left_recorded, len_left_on_wire, core->ipv6_tree);
            ret_val = ret_data.ret_val;
            break;

        default:
            printf("l3 unsoported proto error! %d\n\n", info->mac_info.ether_type);
            ret_val = PROTO_HANDLER_UNSUPPORTED_PROTOCOL;
            break;
    }

    if( ret_val == PROTO_HANDLER_SUCCESS)
    {
        info->ip_info = ret_data.ip_info;
        info->offsets.l4_offset = info->offsets.l3_offset + ret_data.header_len;
        info->offsets.payload_len = ret_data.packet_len;
    }
    return ret_val;
}
