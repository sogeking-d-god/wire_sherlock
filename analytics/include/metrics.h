#ifndef METRICS_H
#define METRICS_H

#include "common.h"

void metric_packet_count(const packet_info_t *pkt, double *target_cell);
void metric_byte_count(const packet_info_t *pkt, double *target_cell);
void metric_syn_flag_count(const packet_info_t *pkt, double *target_cell);
void metric_fin_flag_count(const packet_info_t *pkt, double *target_cell);
void metric_rst_flag_count(const packet_info_t *pkt, double *target_cell);
void metric_psh_flag_count(const packet_info_t *pkt, double *target_cell);
void metric_ack_flag_count(const packet_info_t *pkt, double *target_cell);

#endif