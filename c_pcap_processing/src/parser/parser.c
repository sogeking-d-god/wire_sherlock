#include "parser.h"

//TO-DO: switch from pcap_loop to pcap_next_ex to allow saving the pointer to the packet inside the file

mac_table_t * mac_table_g;
ip_tree_t * ipv4_tree_g;
ip_tree_t * ipv6_tree_g;

parser_return_codes_e parse_pcap_file(const char* file_path)
{
    parser_return_codes_e ret_val = PARSER_SUCCESS;
    flow_table_t * flow_table;

    char error_buffer[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_offline(file_path, error_buffer);
    if (! handle)
    {
        fprintf(stderr, "Could not open pcap file: %s\n", error_buffer);
        ret_val = PARSER_FILE_OPEN_ERROR;
    }
    else
    {
        flow_table = flow_table_init();
        mac_table_g = mac_table_init();
        ipv4_tree_g = ip_tree_init(IPV4_BYTES);
        ipv6_tree_g = ip_tree_init(IPV6_BYTES);

        if(!flow_table)
        {
            fprintf(stderr, "Could not initialize flow table\n");
            ret_val = PARSER_FLOW_TABLE_INIT_ERROR;
        }
        else if(!mac_table_g)
        {
            fprintf(stderr, "Could not initialize mac table\n");
            ret_val = PARSER_MAC_TABLE_INIT_ERROR;
        }
        else if (pcap_loop(handle, 0, advanced_packet_handler, (uint8_t *)flow_table) < 0)
        {
            fprintf(stderr, "Error processing packets: %s\n", pcap_geterr(handle));
            ret_val = PARSER_PACKET_PROCESSING_ERROR;
        }
        else
        {
            // flow_table_print_report(flow_table);
            flow_table_free_table(flow_table);

            // mac_table_print_report(mac_table_g);
            mac_table_free_table(mac_table_g);

            ip_tree_print_report(ipv4_tree_g);
            ip_tree_free_tree(ipv4_tree_g);

            ip_tree_print_report(ipv6_tree_g);
            ip_tree_free_tree(ipv6_tree_g);
        }

        pcap_close(handle);

        printf("---------------------------------\n");
        printf("Finished reading all packets from %s\n", file_path);


    }
    return ret_val;
}

void advanced_packet_handler(uint8_t *args, const struct pcap_pkthdr *header, const uint8_t *packet)
{
    packet_info_t info;
    uint32_t len = header->len;
    flow_table_t * flow_table = (flow_table_t *)args;
    proto_handler_return_codes_e ret_val = handle_l2_packet(packet, len, &info);
    if(ret_val == PROTO_HANDLER_SUCCESS)
    {
        flow_table_process_packet(flow_table, &info);
    }
}

void basic_packet_handler(uint8_t *args, const struct pcap_pkthdr *header, const uint8_t *packet)
{
    struct ethhdr *eth_h;
    struct iphdr *ip_h;
    struct tcphdr *tcp_h;
    struct udphdr *udp_h;
    struct icmphdr *icmp_h;

    char src_ip_str[INET_ADDRSTRLEN];
    char dst_ip_str[INET_ADDRSTRLEN];

    eth_h = (struct ethhdr *)packet;


    if (ntohs(eth_h->h_proto) == ETH_P_IP)
    {
        ip_h = (struct iphdr *)(packet + sizeof(struct ethhdr));

        inet_ntop(AF_INET, &(ip_h->saddr), src_ip_str, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(ip_h->daddr), dst_ip_str, INET_ADDRSTRLEN);


        if (ip_h->protocol == IPPROTO_TCP)
        {
            int ip_header_len = ip_h->ihl * 4;
            tcp_h = (struct tcphdr *)(packet + sizeof(struct ethhdr) + ip_header_len);

            printf("[TCP] %s:%d -> %s:%d (Seq: %u)\n",
                src_ip_str, ntohs(tcp_h->th_sport),
                dst_ip_str, ntohs(tcp_h->th_dport),
                ntohl(tcp_h->th_seq)
            );


        }

        else if (ip_h->protocol == IPPROTO_UDP)
        {
            int ip_header_len = ip_h->ihl * 4;
            udp_h = (struct udphdr *)(packet + sizeof(struct ethhdr) + ip_header_len);

            printf("[UDP] %s:%d -> %s:%d (Len: %hu)\n",
                src_ip_str, ntohs(udp_h->uh_sport),
                dst_ip_str, ntohs(udp_h->uh_dport),
                ntohs(udp_h->uh_ulen)
            );
        }

        else if(ip_h->protocol == IPPROTO_ICMP)
        {
            int ip_header_len = ip_h->ihl * 4;
            icmp_h = (struct icmphdr *)(packet + sizeof(struct ethhdr) + ip_header_len);

            printf("[ICMP] %s -> %s | code: %d | type: %d\n",
                src_ip_str, dst_ip_str,
                icmp_h->code,
                icmp_h->type
            );
        }
    }


}