#include "ip_tree.h"

static ip_tree_diff_bit_ret_t ip_tree_find_diff_bit(uint8_t * new_ip, uint8_t * old_ip, uint8_t starting_bit_index, uint8_t ending_bit_index)
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

        if(ret_val.bit_val != old_ip_bit)
        {
            ret_val.bit_found_flag = TRUE;
            ret_val.bit_index = bit_index;
        }
    }
    return ret_val;
}

ip_tree_t* ip_tree_init(uint8_t ip_addr_byte_count)
{
    ip_tree_t * tree = (ip_tree_t*)calloc(1, IP_TREE_TREE_SIZE);
    if (tree != NULL)
    {
        tree->total_unique_ips = 0;
        tree->ip_addr_byte_count = ip_addr_byte_count;
    }
    return tree;
}

static void ip_tree_free_node(ip_tree_node_t * node, uint8_t byte_count)
{
    if(node != NULL)
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

void ip_tree_free_tree(ip_tree_t* tree)
{
   ip_tree_free_node(tree->root, tree->ip_addr_byte_count);
   free(tree);
}

typedef struct ip_node_test
{
    uint8_t * ip;
    struct ip_node_test * next;
}ip_node_test_t;


ip_node_test_t * ip_arr_g = NULL;

// helper debugging function that checks that we created every ip only once
static ip_node_test_t * check_test(uint8_t *ip, ip_node_test_t ** ipp_array, uint8_t ip_addr_byte_count)
{
    if(!*ipp_array)
    {
        ip_node_test_t * ip_array;

        ip_array = (ip_node_test_t *)malloc(sizeof(ip_node_test_t));
        * ipp_array = ip_array;
        // memccpy( ip_array->ip, ip, 1, tree.ip_addr_byte_count);
        ip_array->ip = ip;
        ip_array->next = NULL;
        return ip_array;
    }
    if(memcmp(ip,(*ipp_array)->ip, ip_addr_byte_count) == 0)
    {
        return NULL;
    }
    else
    {
        return check_test(ip,&((*ipp_array)->next), ip_addr_byte_count);
    }
}


static ip_tree_node_t * ip_tree_create_leaf(uint8_t * ip, uint8_t ip_addr_byte_count, uint16_t packet_len)
{

    ip_tree_node_t * new_leaf_node = (ip_tree_node_t *)calloc(1, IP_TREE_NODE_SIZE);
    uint8_t * ip_addr = (uint8_t *) malloc(ip_addr_byte_count);
    if(new_leaf_node != NULL && ip_addr)
    {
        // new_leaf_node->is_leaf = TRUE;
        new_leaf_node->ip = ip_addr;
        memcpy(ip_addr, ip, ip_addr_byte_count);
        new_leaf_node->stats.total_packets = 1;
        new_leaf_node->stats.total_bytes = packet_len;
        // set to max value so that we check every bit
        new_leaf_node->diff_bit_index = ip_addr_byte_count * BITS_IN_BYTE - 1;


        // calling debugging function
        ip_node_test_t * check_res =  check_test(ip_addr, &ip_arr_g, ip_addr_byte_count);

        if(ip_arr_g == NULL)
        {
            ip_arr_g = check_res;
        }
        if(check_res == NULL)
        {
            printf("badddddd");
        }

    }
    else
    {
        printf("error: tree leaf malloc failed!\n");
    }
    return new_leaf_node;
}


ip_tree_ret_codes_e ip_tree_insert(ip_tree_t *tree, uint8_t * ip, uint32_t packet_len)
{
    ip_tree_ret_codes_e ret_val = IP_TREE_RET_SUCCESS;

    ip_tree_node_t *node = tree->root;
    ip_tree_node_t *new_node = NULL, *new_leaf_node = NULL, ** node_in_tree_pointer = &tree->root;
    ip_tree_diff_bit_ret_t diff_bit;
    uint8_t curr_bit_index = 0;
    boolean_e continue_loop = TRUE;

    //check if the tree is empty
    if(node == NULL)
    {
        new_leaf_node = ip_tree_create_leaf(ip, tree->ip_addr_byte_count, packet_len);
        tree->total_unique_ips++;
        *node_in_tree_pointer = new_leaf_node;
        if(new_leaf_node == NULL)
        {
            printf("error: creating first node in tree");
            ret_val = IP_TREE_RET_MALLOC_FAIL_ERROR;
        }

        //stop from entering the loop
        continue_loop = FALSE;
    }

    while (continue_loop)
    {
        diff_bit = ip_tree_find_diff_bit(ip,node->ip,curr_bit_index,node->diff_bit_index);

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
        else if(diff_bit.bit_index == node->diff_bit_index && node->children[0])
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
            if(!new_leaf_node || !new_node)
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
    return ret_val;
}


static void ip_tree_print_recursive(ip_tree_node_t *node, uint8_t byte_count)
{
    if (node == NULL) return;

    if (node->children[0] == NULL)
    {
        char ip_str[INET6_ADDRSTRLEN];
        int family = (byte_count == 4) ? AF_INET : AF_INET6;

        if (inet_ntop(family, node->ip, ip_str, sizeof(ip_str)))
        {
            printf("IP: %-15s | Packets: %-8u | Bytes: %-10lu\n",
                    ip_str,
                    node->stats.total_packets,
                    node->stats.total_bytes);
        }
    }
    else
    {
        // הוספתי כאן את שם הפונקציה לפני הסוגריים
        ip_tree_print_recursive(node->children[0], byte_count);
        ip_tree_print_recursive(node->children[1], byte_count);
    }
}

void ip_tree_print_report(ip_tree_t *tree)
{
    if (tree == NULL || tree->root == NULL)
    {
        printf("Tree is empty.\n");
        return;
    }

    const char *protocol_type;
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