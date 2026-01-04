#include "print_packet.h"

void print_packet_summary(const packet_info_t *info) {
    char src_ip_str[INET6_ADDRSTRLEN];
    char dst_ip_str[INET6_ADDRSTRLEN];

    if (info->ip_version == IP_VERSION_4)
    {
        inet_ntop(AF_INET, info->src_ip.v4, src_ip_str, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, info->dst_ip.v4, dst_ip_str, INET_ADDRSTRLEN);
    }
    else if (info->ip_version == IP_VERSION_6)
    {
        inet_ntop(AF_INET6, info->src_ip.v6, src_ip_str, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, info->dst_ip.v6, dst_ip_str, INET6_ADDRSTRLEN);
    }


    printf("[%s] %s:%u -> %s:%u (Proto: %u, Len: %u)\n",
           (info->ip_version == IP_VERSION_4 ? "IPv4" : "IPv6"),
           src_ip_str, info->src_port,
           dst_ip_str, info->dst_port,
           info->ip_proto,
           info->packet_len);
}