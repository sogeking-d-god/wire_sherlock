#ifndef LAYER3_STRUCTS_H
#define LAYER3_STRUCTS_H

#include <stdint.h>
#include <sys/time.h>
#include <netinet/in.h>

//unique session identifier
typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  protocol;
} session_key_t;

//session data
typedef struct session_t {
    session_key_t key;
    struct timeval start_time;
    struct timeval last_packet_time;
    uint64_t packet_count;
    uint64_t total_bytes;
    uint32_t retransmission_count;
    struct session_t *next;
} session_t;

#endif