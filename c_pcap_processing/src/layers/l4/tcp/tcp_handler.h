#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H

#include "common.h"


#define TCP_HEADER_MIN_LEN 20
#define TCP_SEQ_LIMIT 1 << 31
#define TCP_SEQ_HALF TCP_SEQ_LIMIT >> 2
#define TCP_OFFSET_IN_BYTES 4

proto_handler_return_codes_e handle_tcp_packet(const uint8_t *data, packet_info_t *info);
flow_table_process_packet_return_e tcp_handler_process_flow_update(flow_node_t *node, packet_info_t *info, const uint8_t *tcp_data, flow_table_first_device_e dev_idx);
#endif