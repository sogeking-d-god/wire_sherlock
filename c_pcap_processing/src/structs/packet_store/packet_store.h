#ifndef PACKET_STORE_H
#define PACKET_STORE_H

#include <unistd.h>
#include <stdlib.h>

#include "common.h"

#define PACKET_STORE_BLOCK_INDEX_GROWTH_FACTOR 2
#define PACKET_STORE_INITIAL_BLOCK_INDEX_CAPACITY 128
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

extern int packet_block_packets_in_chunk_g;

typedef struct packet_block
{
    uint32_t block_id;
    uint32_t packet_count;
    struct timeval start_ts;
    struct timeval end_ts;
    struct packet_block *next;

    packet_info_t *packets;
} packet_block_t;

typedef struct
{
    packet_block_t **block_index; // an array of pointers to packet blocks, sorted by start_ts
    uint32_t total_blocks;
    uint32_t index_capacity;
    packet_block_t *current_block;
} packet_store_t;

typedef enum
{
    PACKET_STORE_SUCCESS = 0,
    PACKET_STORE_ERROR_NULL_ARG = -1,
    PACKET_STORE_ERROR_ALLOC_FAIL = -2,
    PACKET_STORE_ERROR_MEMALIGN_FAIL = -3
} packet_store_status_e;

/**
 * @brief Initializes a new packet store.
 *
 * @return packet_store_t* a pointer to the initialized packet store, or NULL on failure.
 */
packet_store_t* packet_store_init();

/**
 * @brief Adds a packet to the packet store.
 *
 * @param store a pointer to the packet store
 * @param pkt a pointer to the packet to add
 * @return packet_store_status_e the status of the operation
 */
packet_store_status_e packet_store_add_packet(packet_store_t *store, const packet_info_t *pkt);

/**
 * @brief frees all memory associated with the packet store, including all packet blocks and their packets.
 *
 * @param store a pointer to the packet store
 */
void packet_store_free(packet_store_t *store);

/**
 * @brief Calculates the number of packets that can fit in a packet block based on the system's page size and the size of packet_info_t.
 */
void packet_store_calculate_message_count_in_block();


typedef void (*packet_block_callback_fn)(packet_block_t *block, void *context);

/**
 * @brief Compares two time values.
 *
 * @param a a pointer to the first time value
 * @param b a pointer to the second time value
 * @return int -1 if a < b, 0 if a == b, 1 if a > b
 */
int compare_timevals(struct timeval *a, struct timeval *b);

/**
 * @brief The function compares a target time value to the start and end time values of a packet block and determines if the target time is before, within, or after the block's time range.
 *
 * @param target a pointer to the target time value
 * @param block a pointer to the packet block
 * @return int -1 if target is before the block, 0 if target is within the block, 1 if target is after the block
 */
int packet_store_timeval_relative_to_block(struct timeval *target, packet_block_t *block);

/**
 * @brief the function performs a binary search on the packet store's block index to find the block that contains the target time value. It uses the packet_store_timeval_relative_to_block function to compare the target time with the time range of each block.
 *
 * @param store a pointer to the packet store
 * @param target_time a pointer to the target time value
 * @param block_index a pointer to an integer to store the index of the found block
 * @return packet_block_t* a pointer to the found packet block, or NULL if not found
 */
packet_block_t* packet_store_find_block_by_time(packet_store_t *store, struct timeval target_time, int *block_index);

/**
 * @brief the function performs a binary search on the packets within a given block to find the packet that matches the target time value. It uses the compare_timevals function to compare the target time with the capture time of each packet in the block.
 *
 * @param block a pointer to the packet block
 * @param target_time a pointer to the target time value
 * @param result a pointer to store the found packet
 * @return int 0 if found, -1 if not found
 */
int packet_store_find_packet_in_block(packet_block_t *block, struct timeval target_time, packet_info_t* result);

/**
 * @brief the function first uses the packet_store_find_block_by_time function to find the block that contains the target time value. If a block is found, it then calls the packet_store_find_packet_in_block function to search for the specific packet within that block. The function returns the status of the operation and updates the result, block_index, and packet_index parameters with the found packet and its location in the store.
 *
 * @param store a pointer to the packet store
 * @param target_time a pointer to the target time value
 * @param result a pointer to store the found packet
 * @param block_index a pointer to an integer to store the index of the found block
 * @param packet_index a pointer to an integer to store the index of the found packet within the block
 * @return int the status of the operation, 0 if successful, or an error code if not
 */
int packet_store_find_packet_by_time(packet_store_t *store, struct timeval target_time, packet_info_t **result, int * block_index, int * packet_index);

/**
 * @brief Iterates over all packet blocks in the store and calls the provided callback function for each block, passing the block and the context as arguments to the callback.
 *
 * @param store a pointer to the packet store
 * @param callback  a function pointer to the callback function to be called for each block
 * @param context a pointer to any additional context that should be passed to the callback function
 */
void packet_store_itirate_blocks(packet_store_t *store, packet_block_callback_fn callback, void *context);

/**
 * @brief Iterates over all packets in a given block and calls the provided callback function for each packet, passing the packet and the context as arguments to the callback.
 *
 * @param block a pointer to the packet block
 * @param callback a function pointer to the callback function to be called for each packet
 * @param context a pointer to any additional context that should be passed to the callback function
 */
void packet_store_itirate_messages_in_block(packet_block_t *block, packet_block_callback_fn callback, void *context);

/**
 * @brief Iterates over all packets in the store that fall within a specified time range (between start_time and end_time) and calls the provided callback function for each packet, passing the packet and the context as arguments to the callback. The function first finds the block that contains the start_time, then iterates through the blocks and packets until it goes past the end_time, calling the callback for each packet in the range.
 *
 * @param store a pointer to the packet store
 * @param start_time a pointer to the start time value of the range
 * @param end_time a pointer to the end time value of the range
 * @param callback a function pointer to the callback function to be called for each packet in the range
 * @param context a pointer to any additional context that should be passed to the callback function
 */
void packet_store_itirate_packets_in_time_range(packet_store_t *store, struct timeval start_time, struct timeval end_time, packet_block_callback_fn callback, void *context);

#endif