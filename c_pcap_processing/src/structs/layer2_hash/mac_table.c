#include "mac_table.h"

uint8_t empty_mac_addr_g [ETH_ALEN] = {0};


uint32_t mac_table_calculate_hash(const uint8_t *mac)
{
    uint32_t hash = MAC_HASH_CONST;

    for (int i = 0; i < ETH_ALEN; i++)
    {
        // djb2 hash algorithm (each byte is multiplied by 33)
        hash = ((hash << DJB2_SHIFT) + hash) + mac[i];
    }
    return hash % MAC_HASH_SIZE;
}

mac_table_t* mac_table_init()
{
    mac_table_t *table = (mac_table_t*)calloc(MAC_TABLE_SIZE, 1);
    return table;
}

void mac_table_print_report(mac_table_t *table)
{
    if (table)
    {

        printf("\n--- Layer 2 (MAC) Statistics ---\n");
        printf("Unique MACs found: %u\n", table->total_devices);
        printf("%-20s | %-10s | %-15s\n", "MAC Address", "Packets", "Total Bytes");
        printf("------------------------------------------------------------\n");

        for (int i = 0; i < MAC_HASH_SIZE; i++)
        {
            mac_node_t *node = table->buckets[i];
            while (node)
            {
                printf("%02X:%02X:%02X:%02X:%02X:%02X | %-10u | %-15" PRIu64 "\n",
                    node->mac_addr[0], node->mac_addr[1], node->mac_addr[2],
                    node->mac_addr[3], node->mac_addr[4], node->mac_addr[5],
                    node->packet_count, node->total_bytes);
                node = node->next;
            }
        }
    }
}

void mac_table_free_table(mac_table_t *table)
{
    mac_node_t *current_node;
    mac_node_t *next_node;
    if (table)
    {
        for (int i = 0; i < MAC_HASH_SIZE; i++)
        {
            current_node = table->buckets[i];
            while (current_node)
            {
                next_node = current_node->next;
                free(current_node);
                current_node = next_node;
            }
        }

        free(table);
    }
}


mac_table_process_packet_return_e mac_table_process_packet(mac_table_t *table, const mac_table_proc_packet_data_t data)
{
    mac_table_process_packet_return_e ret_val = MAC_TABLE_PROCESS_PACKET_SUCCESS;

    uint32_t hash;
    mac_node_t *node;

    if (table && memcmp(data.mac_addr,empty_mac_addr_g, ETH_ALEN) )
    {
        hash = mac_table_calculate_hash(data.mac_addr);
        node = table->buckets[hash];

        while (node && memcmp(node->mac_addr, data.mac_addr, ETH_ALEN) != 0)
        {
            node = node->next;
        }

        // If node not found create a new one
        if (!node)
        {
            node = (mac_node_t*)calloc(1, MAC_NODE_SIZE);
            if (!node)
            {
                ret_val = MAC_TABLE_PROCESS_PACKET_MALLOC_MAC_NODE_ERROR;
            }
            else
            {
                memcpy(node->mac_addr, data.mac_addr, ETH_ALEN);
                node->packet_count = 0;
                node->total_bytes = 0;

                // Adding the new node as at the head of the bucket
                node->next = table->buckets[hash];
                table->buckets[hash] = node;
                table->total_devices++;
            }
            ret_val = MAC_TABLE_PROCESS_PACKET_ADDED_NEW_NODE_SUCCESS;
        }
        else
        {
            ret_val = MAC_TABLE_PROCESS_PACKET_ADD_TO_EXISTING_MAC_SUCCESS;
        }
        //updating node fields
        node->packet_count++;
        node->total_bytes += data.total_length;

    }
    return ret_val;

}