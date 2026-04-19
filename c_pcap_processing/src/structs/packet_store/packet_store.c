#include <stdio.h>
#include <string.h>

#include "packet_store.h"

int packet_block_packets_in_chunk_g = 0; // Global variable to store the number of packets that can fit in a packet block based on the system's page size and the size of packet_info_t

void packet_store_calculate_message_count_in_block()
{
    size_t header_size;
    size_t available_space;
    if (packet_block_packets_in_chunk_g == 0)
    {
        header_size = sizeof(packet_block_t);
        available_space = PAGE_SIZE - header_size;

        packet_block_packets_in_chunk_g = available_space / sizeof(packet_info_t);
    }
}

packet_store_t* packet_store_init()
{
    packet_store_t *store = (packet_store_t*)malloc(sizeof(packet_store_t));
    if (!store)
    {
        printf("error: packet_store_init: failed to allocate memory for packet_store_t\n");
    }
    else
    {
        store->total_blocks = 0;
        store->index_capacity = PACKET_STORE_INITIAL_BLOCK_INDEX_CAPACITY;
        store->block_index = (packet_block_t**)malloc(sizeof(packet_block_t*) * store->index_capacity);
        if(store->block_index == NULL)
        {
            printf("error: packet_store_init: failed to allocate memory for block index\n");
            free(store);
            store = NULL;
        }
        else
        {
            store->current_block = NULL;
            packet_store_calculate_message_count_in_block();
        }
    }
    return store;
}

/**
 * @brief A helper function to add a new packet block to the packet store when the current block is full or when adding the first packet.
 *
 * @param store a pointer to the packet store
 * @return packet_store_status_e the status of the operation
 */
static packet_store_status_e packet_store_add_block(packet_store_t *store)
{
    packet_store_status_e status;
    packet_block_t *new_block_ptr;
    packet_block_t **new_index;
    uint32_t new_capacity;

    status = PACKET_STORE_SUCCESS;
    new_block_ptr = NULL;

    if (store == NULL)
    {
        status = PACKET_STORE_ERROR_NULL_ARG;
    }

    // Check if we need to grow the block index
    else if (store->total_blocks >= store->index_capacity)
    {
        new_capacity = store->index_capacity * PACKET_STORE_BLOCK_INDEX_GROWTH_FACTOR;
        new_index = (packet_block_t**)realloc(store->block_index, sizeof(packet_block_t*) * new_capacity);

        if (new_index == NULL)
        {
            status = PACKET_STORE_ERROR_ALLOC_FAIL;
            free(store->block_index);
            store->block_index = NULL;
            printf("error: packet_store_add_block: failed to reallocate memory for block index\n");
        }
        else
        {
            store->block_index = new_index;
            store->index_capacity = new_capacity;
        }
    }

    if (status == PACKET_STORE_SUCCESS)
    {
        // malloc a new block with enough space for packet_block_t and packet_info_t array, aligned to page size
        if (posix_memalign((void**)&new_block_ptr, (size_t)PAGE_SIZE, (size_t)PAGE_SIZE) != 0)
        {
            status = PACKET_STORE_ERROR_MEMALIGN_FAIL;
            printf("error: packet_store_add_block: failed to allocate aligned memory for new packet block\n");
        }
        else
        {
            memset(new_block_ptr, 0, (size_t)PAGE_SIZE);

            new_block_ptr->packets = (packet_info_t*)(new_block_ptr + 1);
            new_block_ptr->block_id = store->total_blocks;

            store->block_index[store->total_blocks] = new_block_ptr;

            if (store->current_block != NULL)
            {
                store->current_block->next = new_block_ptr;
            }

            store->current_block = new_block_ptr;
            store->total_blocks++;
        }
    }

    return status;
}

packet_store_status_e packet_store_add_packet(packet_store_t *store, const packet_info_t *pkt)
{
    packet_store_status_e status;
    uint32_t current_idx;

    status = PACKET_STORE_SUCCESS;

    if (store == NULL || pkt == NULL)
    {
        printf("error: packet_store_add_packet: store or pkt argument is NULL\n");
        status = PACKET_STORE_ERROR_NULL_ARG;
    }

    if (status == PACKET_STORE_SUCCESS)
    {
        if (store->current_block == NULL || store->current_block->packet_count >= (uint32_t)packet_block_packets_in_chunk_g)
        {
            status = packet_store_add_block(store);

            if (status == PACKET_STORE_SUCCESS)
            {
                store->current_block->start_ts = pkt->cap_info.ts;
            }
        }
    }

    if (status == PACKET_STORE_SUCCESS)
    {
        current_idx = store->current_block->packet_count;
        store->current_block->packets[current_idx] = *pkt;

        store->current_block->end_ts = pkt->cap_info.ts;
        store->current_block->packet_count++;
    }

    return status;
}

void packet_store_free(packet_store_t *store)
{
    if (store)
    {
        for (uint32_t i = 0; i < store->total_blocks; i++)
        {
            free(store->block_index[i]);
        }
        free(store->block_index);
        free(store);
    }
}

int compare_timevals(struct timeval *a, struct timeval *b)
{
    int retval;

    if (a->tv_sec < b->tv_sec)
    {
        retval = -1;
    }
    else if (a->tv_sec > b->tv_sec)
    {
        retval = 1;
    }
    else
    {
        if (a->tv_usec < b->tv_usec)
        {
            retval = -1;
        }
        else if (a->tv_usec > b->tv_usec)
        {
            retval = 1;
        }
        else
        {
            retval = 0;
        }
    }

    return retval;
}

int packet_store_timeval_relative_to_block(struct timeval *target, packet_block_t *block)
{
    int retval;

    int to_start = compare_timevals(target, &block->start_ts);
    int to_end = compare_timevals(target, &block->end_ts);

    if (to_start < 0)
    {
        retval = -1;
    }
    else if (to_end > 0)
    {
        retval = 1;
    }
    else
    {
        retval = 0;
    }

    return retval;
}

packet_block_t* packet_store_find_block_by_time(packet_store_t *store, struct timeval target_time, int *block_index)
{
    packet_block_t *result;
    int low, mid, high;
    int cmp_result;

    result = NULL;
    low = 0;

    if (block_index != NULL)
    {
        *block_index = -1;
    }

    if (store != NULL && store->total_blocks > 0)
    {
        high = store->total_blocks - 1;

        while (low <= high && !result)
        {
            mid = low + (high - low) / 2;

            cmp_result = packet_store_timeval_relative_to_block(&target_time, store->block_index[mid]);
            if (cmp_result < 0)
            {
                high = mid - 1;
            }
            else if (cmp_result > 0)
            {
                low = mid + 1;
            }
            else
            {
                result = store->block_index[mid];
                if(block_index)
                {
                    *block_index = mid;
                }
            }
        }
    }
    return result;
}

int packet_store_find_packet_in_block(packet_block_t *block, struct timeval target_time, packet_info_t* result)
{
    int retval = -1;

    int low = 0, mid, high;
    int cmp_result;

    result = NULL;

    if(block != NULL &&  packet_store_timeval_relative_to_block(&target_time, block) == 0)
    {
        high = block->packet_count - 1;
        while (low <= high && !result)
        {
            mid = low + (high - low) / 2;

            cmp_result = compare_timevals(&target_time, &block->packets[mid].cap_info.ts);
            if (cmp_result < 0)
            {
                high = mid - 1;
            }
            else if (cmp_result > 0)
            {
                low = mid + 1;
            }
            else
            {
                result = &block->packets[mid];
                retval = mid;
            }
        }

        // if no exact match was found, then mid is the index of the closest time before the target time
        if (result == NULL)
        {
           // If we didn't find an exact match, we will return the packet right before the target time (wich is mid)
           retval = mid;
           result = &block->packets[mid];
        }
    }

    return retval;
}

int packet_store_find_packet_by_time(packet_store_t *store, struct timeval target_time, packet_info_t **result, int * block_index, int * packet_index)
{
    int status;
    packet_block_t *block;

    status = PACKET_STORE_SUCCESS;

    result = NULL;
    *packet_index = -1;

    if (store == NULL)
    {
        printf("error: packet_store_find_packet_by_time: store argument is NULL\n");
        status = PACKET_STORE_ERROR_NULL_ARG;
    }

    if (status == PACKET_STORE_SUCCESS)
    {
        block = packet_store_find_block_by_time(store, target_time, block_index);

        if (block != NULL)
        {
            *packet_index = packet_store_find_packet_in_block(block, target_time, *result);
        }
    }
    return status;
}

void packet_store_itirate_blocks(packet_store_t *store, packet_block_callback_fn callback, void *context)
{
    if (store != NULL && callback != NULL)
    {
        for (int i = 0; i < store->total_blocks; i++)
        {
            callback(store->block_index[i], context);
        }
    }
}

void packet_store_itirate_messages_in_block(packet_block_t *block, packet_block_callback_fn callback, void *context)
{
    if (block != NULL && callback != NULL)
    {
        for (int i = 0; i < block->packet_count; i++)
        {
            callback(&block->packets[i], context);
        }
    }
}

void packet_store_itirate_packets_in_time_range(packet_store_t *store, struct timeval start_time, struct timeval end_time, packet_block_callback_fn callback, void *context)
{
    packet_block_t *block;
    int start_block_index, start_packet_index;
    int end_block_index, end_packet_index;
    int cmp_result;

    packet_block_t * curr_block;
    int p_idx, limit;

    if (store != NULL && callback != NULL)
    {
        // 1. find the first block and packet
        if (packet_store_find_packet_by_time(store, start_time, NULL, &start_block_index, &start_packet_index) == PACKET_STORE_SUCCESS
            && start_block_index != -1
            && start_packet_index != -1)
        {
            block = store->block_index[start_block_index];

            // 2. find the last block and packet
            if (packet_store_find_packet_by_time(store, end_time, NULL, &end_block_index, &end_packet_index) == PACKET_STORE_SUCCESS)
            {
                // if end block index is -1 it means the end time is after the last packet, so we will set it to the last packet in the store
                if (end_block_index == -1 || end_packet_index == -1)
                {
                    end_block_index = store->total_blocks - 1;
                    end_packet_index = store->block_index[end_block_index]->packet_count - 1;
                }

                // 3. iterate over the blocks and packets in the range and call the callback function
                for (int i = start_block_index; i <= end_block_index - 1; i++)
                {
                    packet_store_itirate_messages_in_block(store->block_index[i], callback, context);
                }
                for(int i = 0; i <= end_packet_index; i++)
                {
                    callback(&store->block_index[end_block_index]->packets[i], context);
                }

                for (int b_idx = start_block_index; b_idx <= end_block_index; b_idx++)
                {
                    curr_block = store->block_index[b_idx];

                    // Calculate start index
                    if (b_idx == start_block_index)
                    {
                        p_idx = start_packet_index;
                    }
                    else
                    {
                        p_idx = 0;
                    }

                    // Calculare end index
                    if (b_idx == end_block_index)
                    {
                        limit = end_packet_index;
                    }
                    else
                    {
                        limit = (int)curr_block->packet_count - 1;
                    }

                    for (; p_idx <= limit; p_idx++)
                    {
                        callback(&curr_block->packets[p_idx], context);
                    }
                }
            }
        }
    }
}