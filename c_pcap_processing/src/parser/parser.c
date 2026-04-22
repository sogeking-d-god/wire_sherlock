#include "parser.h"
#include "l4_handler.h"
#include "time_series.h"

void init_core(file_analysis_context_t *core)
{
    *core = (file_analysis_context_t){0};

    core->flow_table = flow_table_init();
    core->mac_table = mac_table_init();
    core->ipv4_tree = ip_tree_init(IPV4_BYTES);
    core->ipv6_tree = ip_tree_init(IPV6_BYTES);
    core->packet_store = packet_store_init();
    core->bin_manager = (bin_manager_t*)calloc(1, sizeof(bin_manager_t));
}


parser_return_codes_e parse_pcap_file(file_analysis_context_t *core, const char* file_path)
{
    parser_return_codes_e ret_val = PARSER_SUCCESS;

    bin_manager_ret_e bin_mgr_ret;

    struct pcap_pkthdr *header;
    const uint8_t *packet;
    int res;

    uint32_t current_file_offset = PCAP_FILE_HEADER_SIZE;

    char error_buffer[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_offline(file_path, error_buffer);

    if (! handle)
    {
        fprintf(stderr, "Could not open pcap file: %s\n", error_buffer);
        ret_val = PARSER_FILE_OPEN_ERROR;
    }
    else
    {
        init_core(core);
        if(!core->flow_table || !core->mac_table || !core->ipv4_tree || !core->ipv6_tree || !core->packet_store || !core->bin_manager)
        {
            fprintf(stderr, "Could not initialize one or more tables\n");
        }
        else
        {
            while ((res = pcap_next_ex(handle, &header, &packet)) >= 0)
            {
                if (core->total_packets == 0)
                {
                    core->start_ts = header->ts;
                }
                core->end_ts = header->ts;
                core->total_packets++;

                advanced_packet_handler(core, header, packet, current_file_offset);

                // Update the file offset for the next packet (packet header + packet data)
                current_file_offset += 16 + header->caplen;
            }

            if (res == -1)
            {
                fprintf(stderr, "Error reading the packets: %s\n", pcap_geterr(handle));
                ret_val = PARSER_PACKET_PROCESSING_ERROR;
            }

            packet_store_debug_print(core->packet_store);
            // flow_table_print_report(core->flow_table);
            // mac_table_print_report(core->mac_table);
            // ip_tree_print_report(core->ipv4_tree);
            // ip_tree_print_report(core->ipv6_tree);
        }

        pcap_close(handle);

        printf("---------------------------------\n");
        printf("Finished reading all packets from %s\n", file_path);

        printf("Calculating time series metrics...\n");

        core->bin_manager->start_ts = core->start_ts;
        core->bin_manager->end_ts = core->end_ts;
        bin_mgr_ret = bin_manager_init(core->bin_manager);

        if (bin_mgr_ret != BIN_MANAGER_SUCCESS)
        {
            fprintf(stderr, "Error initializing bin manager: %d\n", bin_mgr_ret);
        }
        else
        {
            packet_store_itirate_all_packets(core->packet_store, packet_store_time_series_callback_fn, core->bin_manager);

            printf("Time series metrics calculated successfully.\n");
        }

    }
    return ret_val;
}

static ip_tree_ret_codes_e parser_insert_to_mac_ip_tree(ip_addr_t ip_addr, ip_version_e ip_ver, mac_node_t * mac_node, uint32_t packet_len)
{
    ip_tree_ret_codes_e ret_val;
    if(ip_ver == IP_VERSION_4)
    {
        ret_val = ip_tree_insert(mac_node->ipv4_tree, ip_addr.v4, packet_len);
    }
    else
    {
        ret_val = ip_tree_insert(mac_node->ipv6_tree, ip_addr.v6, packet_len);
    }
    return ret_val;
}

void advanced_packet_handler(file_analysis_context_t *core, const struct pcap_pkthdr *header, const uint8_t *packet, uint32_t file_offset)
{
    packet_info_t info = {0};
    proto_handler_return_codes_e ret_val;
    mac_table_proc_packet_data_t mac_data;
    mac_table_proc_packet_ret_t src_mac_ret;
    mac_table_proc_packet_ret_t dst_mac_ret;
    flow_table_process_packet_ret_t flow_ret;

    info.cap_info.ts = header->ts;
    info.cap_info.caplen = header->caplen;
    info.cap_info.wire_len = header->len;

    info.offsets.packet_start_pointer = file_offset;

    ret_val = handle_l2_packet(packet, &info);
    if(ret_val == PROTO_HANDLER_SUCCESS)
    {
        //insert to mac table
        mac_data.total_length = info.cap_info.wire_len;
        //for src mac
        memcpy(mac_data.mac_addr, info.mac_info.src_mac, ETH_ALEN);
        src_mac_ret = mac_table_process_packet(core->mac_table, mac_data);

        //for dest mac
        memcpy(mac_data.mac_addr, info.mac_info.dst_mac, ETH_ALEN);
        dst_mac_ret = mac_table_process_packet(core->mac_table, mac_data);

        ret_val = handle_l3_packet(packet + info.offsets.l3_offset, &info, core);
    }
    if(ret_val == PROTO_HANDLER_SUCCESS)
    {
        if(src_mac_ret.ret_code >= MAC_TABLE_PROCESS_PACKET_SUCCESS)
        {
            parser_insert_to_mac_ip_tree(info.ip_info.src_ip, info.ip_info.ip_version, src_mac_ret.node, info.cap_info.wire_len);
        }

        if(dst_mac_ret.ret_code >= MAC_TABLE_PROCESS_PACKET_SUCCESS)
        {
            parser_insert_to_mac_ip_tree(info.ip_info.dst_ip, info.ip_info.ip_version, dst_mac_ret.node, info.cap_info.wire_len);
        }

        ret_val = handle_l4_packet(packet + info.offsets.l4_offset, &info);
    }

    if(ret_val == PROTO_HANDLER_SUCCESS)
    {
        // print_packet_summary(info);

        flow_ret = flow_table_process_packet(core->flow_table, &info);
        if(flow_ret.ret_code >= FLOW_TABLE_PROCESS_PACKET_SUCCESS)
        {

            // TCP messages are handled by tcp handler
            if(flow_ret.flow_node_ptr->key.protocol == IPPROTO_TCP )
            {
                ret_val = (int)tcp_handler_process_flow_update(flow_ret.flow_node_ptr, &info,  packet + info.offsets.l4_offset, flow_ret.dev_idx);
            }
            else
            {
                ret_val = (int)flow_table_insert_to_session(flow_ret.flow_node_ptr, &info, flow_ret.dev_idx);
            }
        }
        else
        {
            fprintf(stderr, "DEBUG: Flow table error: %d\n", flow_ret.ret_code);
        }
    }

    // If the packet was successfully processed by all handlers, add it to the packet store for time series analysis
    if(ret_val != PROTO_HANDLER_CORRUPT_PACKET)
    {
        packet_store_add_packet(core->packet_store, &info);
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


void core_free(file_analysis_context_t *core)
{
    if (core)
    {
        if (core->flow_table)
        {
            flow_table_free_table(core->flow_table);
        }
        if (core->mac_table)
        {
            mac_table_free_table(core->mac_table);
        }
        if (core->ipv4_tree)
        {
            ip_tree_free_tree(core->ipv4_tree);
        }
        if (core->ipv6_tree)
        {
            ip_tree_free_tree(core->ipv6_tree);
        }
        if (core->packet_store)
        {
            packet_store_free(core->packet_store);
        }
        if(core->bin_manager)
        {
            bin_manager_free(core->bin_manager);
        }

        free(core);
    }
}
