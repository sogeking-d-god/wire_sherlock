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