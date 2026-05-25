#include "mac_table.h"
#include "xxhash.h"

uint8_t empty_mac_addr_g[ETH_ALEN] = {0};

/**
 * @brief Computes the hash table bucket index for a MAC address.
 *
 * @param mac_addr Pointer to the 6-byte MAC address.
 * @return Bucket index in range [0, MAC_HASH_SIZE).
 */
uint32_t mac_table_calculate_hash(const uint8_t *mac_addr)
{
    return (uint32_t)(XXH3_64bits(mac_addr, ETH_ALEN) & (MAC_HASH_SIZE - 1));
}

/**
 * @brief Allocates and initialises an empty MAC table.
 *
 * @return Pointer to the new table, or NULL on allocation failure.
 */
mac_table_t *mac_table_init()
{
    mac_table_t *table = (mac_table_t *)calloc(1, MAC_TABLE_SIZE);

    return table;
}

/**
 * @brief Prints a human-readable summary of all MAC entries to stdout.
 *
 * @param table Pointer to the MAC table.
 */
void mac_table_print_report(mac_table_t *table)
{
    if (table)
    {
        printf("\n--- Layer 2 (MAC) Statistics ---\n");
        printf("Unique MACs found: %u\n", table->total_devices);
        printf("%-20s | %-10s | %-15s\n", "MAC Address", "Packets", "Total Bytes");
        printf("------------------------------------------------------------\n");

        for (int bucket_idx = 0; bucket_idx < MAC_HASH_SIZE; bucket_idx++)
        {
            mac_node_t *node = table->buckets[bucket_idx];

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

/**
 * @brief Frees all memory owned by the MAC table, including all nodes and IP trees.
 *
 * @param table Pointer to the MAC table to free.
 */
void mac_table_free_table(mac_table_t *table)
{
    mac_node_t *current_node;
    mac_node_t *next_node;

    if (table)
    {
        for (int bucket_idx = 0; bucket_idx < MAC_HASH_SIZE; bucket_idx++)
        {
            current_node = table->buckets[bucket_idx];

            while (current_node)
            {
                next_node = current_node->next;

                ip_tree_free_tree(current_node->ipv4_tree);
                ip_tree_free_tree(current_node->ipv6_tree);
                free(current_node);

                current_node = next_node;
            }
        }

        free(table);
    }
}

/**
 * @brief Looks up or creates the MAC node for an incoming packet, then updates counters.
 *
 * Skips all-zero (empty) MAC addresses. On a cache miss a new node is allocated
 * and both an IPv4 and an IPv6 patricia tree are initialised for it.
 *
 * @param table The MAC hash table.
 * @param data Packet data containing the MAC address and total length.
 * @return A struct containing the return code and a pointer to the matched/new node.
 */
mac_table_proc_packet_ret_t mac_table_process_packet(mac_table_t *table, const mac_table_proc_packet_data_t data)
{
    mac_table_proc_packet_ret_t ret_struct;
    uint32_t hash;
    mac_node_t *node;

    ret_struct.ret_code = MAC_TABLE_PROCESS_PACKET_SUCCESS;

    if (table && memcmp(data.mac_addr, empty_mac_addr_g, ETH_ALEN))
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
            node = (mac_node_t *)calloc(1, MAC_NODE_SIZE);

            if (!node)
            {
                ret_struct.ret_code = MAC_TABLE_PROCESS_PACKET_MALLOC_MAC_NODE_ERROR;
                printf("mac table malloc error!\n");
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

                // initing trees
                node->ipv4_tree = ip_tree_init(IPV4_BYTES);

                if (!node->ipv4_tree)
                {
                    printf("error failed to init ipv4 tree for mac\n");
                    ret_struct.ret_code = MAC_TABLE_PROCESS_PACKET_INIT_IP_TREE_ERROR;
                }
                else
                {
                    node->ipv6_tree = ip_tree_init(IPV6_BYTES);

                    if (!node->ipv6_tree)
                    {
                        printf("error failed to init ipv6 tree for mac\n");
                        ret_struct.ret_code = MAC_TABLE_PROCESS_PACKET_INIT_IP_TREE_ERROR;
                        ip_tree_free_tree(node->ipv4_tree);
                        node->ipv4_tree = NULL;
                    }
                }

                ret_struct.ret_code = MAC_TABLE_PROCESS_PACKET_ADDED_NEW_NODE_SUCCESS;
            }
        }
        else
        {
            ret_struct.ret_code = MAC_TABLE_PROCESS_PACKET_ADD_TO_EXISTING_MAC_SUCCESS;
        }

        if (ret_struct.ret_code >= MAC_TABLE_PROCESS_PACKET_SUCCESS)
        {
            //updating node fields
            node->packet_count++;
            node->total_bytes += data.total_length;
            ret_struct.node = node;
        }
    }

    return ret_struct;
}

/**
 * @brief Iterates over every MAC node in the table and invokes a callback.
 *
 * @param table The MAC table to iterate.
 * @param callback Function called for each MAC node.
 * @param context Caller-supplied context pointer forwarded to the callback.
 */
void mac_table_iterate(mac_table_t *table, mac_node_callback_fn callback, void *context)
{
    mac_node_t *curr_node;
    mac_node_t *next_node;

    if (table && callback)
    {
        for (int bucket_idx = 0; bucket_idx < MAC_HASH_SIZE; bucket_idx++)
        {
            curr_node = table->buckets[bucket_idx];

            while (curr_node)
            {
                next_node = curr_node->next;
                callback(curr_node, context);
                curr_node = next_node;
            }
        }
    }
}
