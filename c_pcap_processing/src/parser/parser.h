#ifndef PARSER_H
#define PARSER_H

#include <pcap.h>

#include "common.h"
#include "../layers/l2/eth_handler.h"

#include "flow_table.h"
#include "mac_table.h"

#include "time_series.h"
#include "packet_store.h"

#define PCAP_FILE_HEADER_SIZE 24
#define PCAP_PACKET_HEADER_SIZE 16

typedef struct packet_store packet_store_t;
typedef struct bin_manager bin_manager_t;


typedef enum
{
    PARSER_SUCCESS = 0,
    PARSER_FILE_OPEN_ERROR = -1,
    PARSER_PACKET_PROCESSING_ERROR = -2,
    PARSER_FLOW_TABLE_INIT_ERROR = -3,
    PARSER_MAC_TABLE_INIT_ERROR = -4,
} parser_return_codes_e;

typedef struct file_analysis_context_s
{
    // Parsing Data
    flow_table_t *flow_table;
    mac_table_t  *mac_table;
    ip_tree_t    *ipv4_tree;
    ip_tree_t    *ipv6_tree;

    // Meta-Data of the PCAP
    struct timeval start_ts;
    struct timeval end_ts;
    uint64_t total_packets;

    // Time series data
    packet_store_t * packet_store;
    bin_manager_t * bin_manager;

    // Anomaly caches populated by cmd_generate_anomalies and consumed by
    // cmd_cluster_anomalies (Hardening Constraint #1 validates these before deref).
    // Opaque void* so parser.h does not depend on analytics headers; the
    // anomaly_wrapper layer casts these to scored_segments_list_t* /
    // micro_events_list_t* and frees them via anomaly_wrapper_free_caches,
    // which engine_controller calls right before core_free.
    void *anomaly_macro_cache;
    void *anomaly_micro_cache;
} file_analysis_context_t;

parser_return_codes_e parse_pcap_file(file_analysis_context_t *core, const char* file_path);

void basic_packet_handler(uint8_t *args, const struct pcap_pkthdr *header, const uint8_t *packet);

void advanced_packet_handler(file_analysis_context_t *core, const struct pcap_pkthdr *header, const uint8_t *packet, uint32_t file_offset);

void core_free(file_analysis_context_t *core);


#endif