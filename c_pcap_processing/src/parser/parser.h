#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "../layers/l2/eth_handler.h"
#include <pcap.h>

typedef enum
{
    PARSER_SUCCESS = 0,
    PARSER_FILE_OPEN_ERROR = -1,
    PARSER_PACKET_PROCESSING_ERROR = -2
} parser_return_codes_e;

parser_return_codes_e parse_pcap_file(const char* file_path);

void basic_packet_handler(uint8_t *args, const struct pcap_pkthdr *header, const uint8_t *packet);

void advanced_packet_handler(uint8_t *args, const struct pcap_pkthdr *header, const uint8_t *packet);

#endif