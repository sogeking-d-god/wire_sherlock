#include "metrics.h"

void metric_packet_count(const packet_info_t *pkt, double *target_cell)
{
    (*target_cell)++;
}

void metric_byte_count(const packet_info_t *pkt, double *target_cell)
{
    (*target_cell) += pkt->offsets.payload_len;
}

void metric_syn_flag_count(const packet_info_t *pkt, double *target_cell)
{
    if (pkt->port_info.tcp_flags & TH_SYN)
    {
        (*target_cell)++;
    }
}

void metric_fin_flag_count(const packet_info_t *pkt, double *target_cell)
{
    if (pkt->port_info.tcp_flags & TH_FIN)
    {
        (*target_cell)++;
    }
}

void metric_rst_flag_count(const packet_info_t *pkt, double *target_cell)
{
    if (pkt->port_info.tcp_flags & TH_RST)
    {
        (*target_cell)++;
    }
}

void metric_psh_flag_count(const packet_info_t *pkt, double *target_cell)
{
    if (pkt->port_info.tcp_flags & TH_PUSH)
    {
        (*target_cell)++;
    }
}

void metric_ack_flag_count(const packet_info_t *pkt, double *target_cell)
{
    if (pkt->port_info.tcp_flags & TH_ACK)
    {
        (*target_cell)++;
    }
}

void metric_retransmit_count(const packet_info_t *pkt, double *target_cell)
{
    if (pkt->is_retransmit == TRUE)
    {
        (*target_cell)++;
    }
}