#include "ip_tree.h"

/**
 * @brief a helper function that finds the first bit index where 2 ips differ, and the value of that bit in the new ip.
 *  if the ips are the same in the given range, it returns ending_bit_index + 1 as the bit index and FALSE as the flag
 *
 * @param new_ip first ip to compare
 * @param old_ip second ip to compare
 * @param starting_bit_index the bit index to start the comparison from (inclusive)
 * @param ending_bit_index the bit index to end the comparison at (inclusive)
 * @return ip_tree_diff_bit_ret_t a struct that contains the index of the first different bit, the value of that bit in the new ip, and a flag indicating if a different bit was found
 */
static ip_tree_diff_bit_ret_t ip_tree_find_diff_bit(uint8_t *new_ip, uint8_t *old_ip, uint8_t starting_bit_index, uint8_t ending_bit_index)
{
    ip_tree_diff_bit_ret_t ret_val = {0};
    uint8_t byte_index;
    uint16_t bit_index;
    uint8_t old_ip_bit;
    uint8_t bit_mask;

    ret_val.bit_found_flag = FALSE;
    ret_val.bit_index = ending_bit_index;

    // the loop includes the upper limit since we want to know the val of its bit to know to wich child to send the new ip to (0 or 1)
    for (bit_index = starting_bit_index; bit_index <= ending_bit_index && ret_val.bit_found_flag == FALSE; bit_index++)
    {
        bit_mask = IP_TREE_BIT_MASK_START_VAL >> (bit_index % BITS_IN_BYTE);
        byte_index = bit_index / BITS_IN_BYTE;
        ret_val.bit_val = (new_ip[byte_index] & bit_mask) ? 1 : 0;
        old_ip_bit = (old_ip[byte_index] & bit_mask) ? 1 : 0;

        if (ret_val.bit_val != old_ip_bit)
        {
            ret_val.bit_found_flag = TRUE;
            ret_val.bit_index = bit_index;
        }
    }

    return ret_val;
}

/**
 * @brief Allocates and initialises an empty patricia tree for IP addresses.
 *
 * @param ip_addr_byte_count Number of bytes in each IP address (4 for IPv4, 16 for IPv6).
 * @return Pointer to the new tree, or NULL on allocation failure.
 */
ip_tree_t *ip_tree_init(uint8_t ip_addr_byte_count)
{
    ip_tree_t *tree = (ip_tree_t *)calloc(1, IP_TREE_TREE_SIZE);

    if (tree != NULL)
    {
        tree->total_unique_ips = 0;
        tree->ip_addr_byte_count = ip_addr_byte_count;
    }

    return tree;
}

/**
 * @brief Recursively frees all nodes in the tree.
 *
 * @param node The current node to free.
 * @param byte_count Number of bytes in each IP address.
 */
static void ip_tree_free_node(ip_tree_node_t *node, uint8_t byte_count)
{
    if (node != NULL)
    {
        // if its a leaf
        if (node->children[0] == NULL)
        {
            free(node->ip);
        }

        ip_tree_free_node(node->children[0], byte_count);
        ip_tree_free_node(node->children[1], byte_count);
        free(node);
    }
}

/**
 * @brief Frees the entire patricia tree and all its nodes.
 *
 * @param tree Pointer to the tree to free.
 */
void ip_tree_free_tree(ip_tree_t *tree)
{
    if (tree)
    {
        ip_tree_free_node(tree->root, tree->ip_addr_byte_count);
        free(tree);
    }
}

/**
 * @brief Allocates a new leaf node for the given IP address.
 *
 * @param ip Pointer to the IP address bytes.
 * @param ip_addr_byte_count Number of bytes in the IP address.
 * @param packet_len Length of the packet that triggered this insert.
 * @return Pointer to the new leaf node, or NULL on allocation failure.
 */
static ip_tree_node_t *ip_tree_create_leaf(uint8_t *ip, uint8_t ip_addr_byte_count, uint16_t packet_len)
{
    ip_tree_node_t *new_leaf_node = (ip_tree_node_t *)calloc(1, IP_TREE_NODE_SIZE);
    uint8_t *ip_addr = (uint8_t *)malloc(ip_addr_byte_count);

    if (new_leaf_node != NULL && ip_addr)
    {
        // new_leaf_node->is_leaf = TRUE;
        new_leaf_node->ip = ip_addr;
        memcpy(ip_addr, ip, ip_addr_byte_count);
        new_leaf_node->stats.total_packets = 1;
        new_leaf_node->stats.total_bytes = packet_len;
        // set to max value so that we check every bit
        new_leaf_node->diff_bit_index = ip_addr_byte_count * BITS_IN_BYTE - 1;
    }
    else
    {
        printf("error: tree leaf malloc failed!\n");
    }

    return new_leaf_node;
}

/**
 * @brief Inserts an IP address into the patricia tree, or updates its stats if it already exists.
 *
 * @param tree Pointer to the patricia tree.
 * @param ip Pointer to the IP address bytes to insert.
 * @param packet_len Length of the packet associated with this IP.
 * @return IP_TREE_RET_SUCCESS on success, or an error code on failure.
 */
ip_tree_ret_codes_e ip_tree_insert(ip_tree_t *tree, uint8_t *ip, uint32_t packet_len)
{
    ip_tree_ret_codes_e ret_val = IP_TREE_RET_SUCCESS;
    ip_tree_node_t *node;
    ip_tree_node_t *new_node;
    ip_tree_node_t *new_leaf_node;
    ip_tree_node_t **node_in_tree_pointer;
    ip_tree_diff_bit_ret_t diff_bit;
    uint8_t curr_bit_index;
    boolean_e continue_loop;

    new_node = NULL;
    new_leaf_node = NULL;
    curr_bit_index = 0;
    continue_loop = TRUE;

    if (!tree)
    {
        printf("error: ip tree is NULL error\n");
        ret_val = IP_TREE_RET_NULL_TREE_ERROR;
    }
    else
    {
        node = tree->root;
        node_in_tree_pointer = &tree->root;

        //check if the tree is empty
        if (node == NULL)
        {
            new_leaf_node = ip_tree_create_leaf(ip, tree->ip_addr_byte_count, packet_len);
            tree->total_unique_ips++;
            *node_in_tree_pointer = new_leaf_node;

            if (new_leaf_node == NULL)
            {
                printf("error: creating first node in tree");
                ret_val = IP_TREE_RET_MALLOC_FAIL_ERROR;
            }

            //stop from entering the loop
            continue_loop = FALSE;
        }

        while (continue_loop)
        {
            diff_bit = ip_tree_find_diff_bit(ip, node->ip, curr_bit_index, node->diff_bit_index);

            // if we reached a leaf node with the same ip
            if (!node->children[0] && diff_bit.bit_found_flag == FALSE)
            {
                node->stats.total_packets++;
                node->stats.total_bytes += packet_len;

                continue_loop = FALSE;
            }
            /* if the diff_bit of the new ip is AFTER the diff_bit of the children of the node.

            so the new ip is BELOW the level of the node
            */
            else if (diff_bit.bit_index == node->diff_bit_index && node->children[0])
            {
                curr_bit_index = node->diff_bit_index + 1;

                // Check and handle special case to avoid seg fault of getting out of the range of the ip address
                // (hapends when there are 2 ip addresses that differ only in the last bit)
                if (curr_bit_index >= tree->ip_addr_byte_count * BITS_IN_BYTE)
                {
                    curr_bit_index = node->diff_bit_index;

                    // printf("curr_bit_index: %u, node->diff_bit_index: %u\n", curr_bit_index, node->diff_bit_index);
                    // printf("new ip: %d.%d.%d.%d, old leaf: %d, children: %p, %p ip: %d.%d.%d.%d\n",
                    //         ip[0], ip[1], ip[2], ip[3],
                    //         node->is_leaf,
                    //         node->children[0], node->children[1],
                    //         node->ip[0], node->ip[1], node->ip[2], node->ip[3]);
                }

                node_in_tree_pointer = &node->children[diff_bit.bit_val];
                node = *node_in_tree_pointer;
            }
            /* if the diff_bit of the new ip is BEFORE the diff_bit of the children of the node.
            so the new ip is AT the level of the node
            */
            else
            {
                new_leaf_node = ip_tree_create_leaf(ip, tree->ip_addr_byte_count, packet_len);
                new_node = (ip_tree_node_t *)calloc(1, IP_TREE_NODE_SIZE);

                if (!new_leaf_node || !new_node)
                {
                    ret_val = IP_TREE_RET_MALLOC_FAIL_ERROR;
                    printf("malloc error\n");
                }
                else
                {
                    tree->total_unique_ips++;
                    new_node->diff_bit_index = diff_bit.bit_index;
                    // new_node->is_leaf = FALSE;
                    new_node->ip = new_leaf_node->ip;

                    new_node->children[diff_bit.bit_val] = new_leaf_node;
                    new_node->children[!diff_bit.bit_val] = node;

                    *node_in_tree_pointer = new_node;
                }

                continue_loop = FALSE;
            }
        }
    }

    return ret_val;
}

/**
 * @brief Recursively prints all leaf nodes (unique IPs) in the subtree.
 *
 * @param node Current node in the traversal.
 * @param byte_count Number of bytes in the IP address (4 or 16).
 */
static void ip_tree_print_recursive(ip_tree_node_t *node, uint8_t byte_count)
{
    if (node != NULL)
    {
        if (node->children[0] == NULL)
        {
            char ip_str[INET6_ADDRSTRLEN];
            int addr_family = (byte_count == 4) ? AF_INET : AF_INET6;

            if (inet_ntop(addr_family, node->ip, ip_str, sizeof(ip_str)))
            {
                printf("IP: %-15s | Packets: %-8u | Bytes: %-10lu\n",
                    ip_str,
                    node->stats.total_packets,
                    node->stats.total_bytes);
            }
        }
        else
        {
            ip_tree_print_recursive(node->children[0], byte_count);
            ip_tree_print_recursive(node->children[1], byte_count);
        }
    }
}

/**
 * @brief Prints a human-readable report of all IPs stored in the tree.
 *
 * @param tree Pointer to the patricia tree.
 */
void ip_tree_print_report(ip_tree_t *tree)
{
    const char *protocol_type;

    if (tree == NULL || tree->root == NULL)
    {
        printf("Tree is empty.\n");
    }
    else
    {
        if (tree->ip_addr_byte_count == 4)
        {
            protocol_type = "IPv4";
        }
        else if (tree->ip_addr_byte_count == 16)
        {
            protocol_type = "IPv6";
        }
        else
        {
            protocol_type = "Unknown";
        }

        printf("\n--- %s (%d bytes) Traffic Report ---\n", protocol_type, tree->ip_addr_byte_count);
        printf("Total Unique IPs: %u\n", tree->total_unique_ips);
        printf("--------------------------------------------\n");

        ip_tree_print_recursive(tree->root, tree->ip_addr_byte_count);

        printf("--------------------------------------------\n");
    }
}

/**
 * @brief Recursively iterates over all leaf nodes and invokes a callback for each.
 *
 * @param node Current node in the traversal.
 * @param callback Function called for each leaf node.
 * @param ip_addr_type IP version (IPv4 or IPv6) passed to the callback.
 * @param context Caller-supplied context pointer forwarded to the callback.
 */
static void ip_tree_iterate_recursive(ip_tree_node_t *node, ip_node_callback_fn callback, ip_version_e ip_addr_type, void *context)
{
    if (node != NULL)
    {
        // Check if it's a leaf node (no children)
        if (node->children[0] == NULL)
        {
            callback(node, ip_addr_type, context);
        }
        else
        {
            ip_tree_iterate_recursive(node->children[0], callback, ip_addr_type, context);
            ip_tree_iterate_recursive(node->children[1], callback, ip_addr_type, context);
        }
    }
}

/**
 * @brief Iterates over all leaf nodes in the tree and invokes a callback for each.
 *
 * @param tree Pointer to the patricia tree.
 * @param callback Function called for each unique IP node.
 * @param context Caller-supplied context pointer forwarded to the callback.
 */
void ip_tree_iterate(ip_tree_t *tree, ip_node_callback_fn callback, void *context)
{
    int ip_addr_type;

    if (tree && callback)
    {
        ip_addr_type = (tree->ip_addr_byte_count == 4) ? IP_VERSION_4 : IP_VERSION_6;
        ip_tree_iterate_recursive(tree->root, callback, ip_addr_type, context);
    }
}

/**
 * @brief a helper function that checks if the first prefix_len bits of 2 ips match
 *
 * @param ip_a first ip to compare
 * @param ip_b second ip to compare
 * @param prefix_len the number of bits to compare from the most significant bit
 * @return boolean_e TRUE if the bits match, FALSE otherwise
 */
static boolean_e ip_tree_prefix_matches(const uint8_t *ip_a, const uint8_t *ip_b, uint8_t prefix_len)
{
    uint8_t full_bytes = prefix_len / BITS_IN_BYTE;
    uint8_t remaining_bits = prefix_len % BITS_IN_BYTE;
    uint8_t tail_mask;
    boolean_e ret_val = TRUE;

    if (full_bytes > 0 && memcmp(ip_a, ip_b, full_bytes) != 0)
    {
        ret_val = FALSE;
    }
    else if (remaining_bits > 0)
    {
        tail_mask = (uint8_t)(0xFF << (BITS_IN_BYTE - remaining_bits));
        if ((ip_a[full_bytes] & tail_mask) != (ip_b[full_bytes] & tail_mask))
        {
            ret_val = FALSE;
        }
    }

    return ret_val;
}

/**
 * @brief Finds all IPs in the tree that belong to the given subnet and invokes a callback for each.
 *
 * @param tree Pointer to the patricia tree.
 * @param subnet_ip Pointer to the subnet IP address bytes.
 * @param prefix_len Number of prefix bits that define the subnet mask.
 * @param callback Function called for each matching IP node.
 * @param context Caller-supplied context pointer forwarded to the callback.
 * @return IP_TREE_RET_SUCCESS on success, or an error code on invalid input.
 */
ip_tree_ret_codes_e ip_tree_find_subnet(ip_tree_t *tree,
                                        const uint8_t *subnet_ip,
                                        uint8_t prefix_len,
                                        ip_node_callback_fn callback,
                                        void *context)
{
    ip_tree_node_t *node;
    uint16_t total_bits;
    uint8_t bit_mask;
    uint8_t byte_index;
    uint8_t bit_val;
    ip_version_e ip_addr_type;
    ip_tree_ret_codes_e ret_val = IP_TREE_RET_SUCCESS;

    if (!tree || !subnet_ip || !callback)
    {
        ret_val = IP_TREE_RET_NULL_TREE_ERROR;
    }
    else
    {
        total_bits = (uint16_t)tree->ip_addr_byte_count * BITS_IN_BYTE;

        if (prefix_len > total_bits)
        {
            ret_val = IP_TREE_RET_INVALID_PREFIX_ERROR;
        }
        else if (tree->root != NULL)
        {
            // go down the tree until we reach a node where the next bit to check is after the prefix_len, or we reach a leaf
            node = tree->root;
            while (node->children[0] != NULL && node->diff_bit_index < prefix_len)
            {
                byte_index = node->diff_bit_index / BITS_IN_BYTE;
                bit_mask = IP_TREE_BIT_MASK_START_VAL >> (node->diff_bit_index % BITS_IN_BYTE);
                bit_val = (subnet_ip[byte_index] & bit_mask) ? 1 : 0;
                node = node->children[bit_val];
            }

            // check if the node we reached matches the prefix, if not - return since there are no ips in the tree that match the prefix
            if (prefix_len == 0 || ip_tree_prefix_matches(node->ip, subnet_ip, prefix_len))
            {
                ip_addr_type = (tree->ip_addr_byte_count == IPV4_BYTES) ? IP_VERSION_4 : IP_VERSION_6;
                ip_tree_iterate_recursive(node, callback, ip_addr_type, context);
            }
        }
    }

    return ret_val;
}
