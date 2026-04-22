#ifndef TIME_SERIES_H
#define TIME_SERIES_H

#include "common.h"
#include "metrics.h"
#include "packet_store.h"

#define DEFAULT_BIN_SIZE 100 // 100 milliseconds
#define MILLISECONDS_IN_SECOND 1000
#define MICROSECONDS_IN_MILLISECOND 1000
typedef enum
{
    METRIC_PACKET_COUNT = 0,
    METRIC_BYTE_COUNT = 1,
    METRIC_SYN_FLAG_COUNT = 2,
    METRIC_FIN_FLAG_COUNT = 3,
    METRIC_RST_FLAG_COUNT = 4,
    METRICS_COUNT
} metric_index_e;

#define ALL_METRICS ((metric_index_e[]){METRIC_PACKET_COUNT, METRIC_BYTE_COUNT, METRIC_SYN_FLAG_COUNT, METRIC_FIN_FLAG_COUNT, METRIC_RST_FLAG_COUNT})

typedef void (*metric_fn)(const packet_info_t *pkt, double *target_cell);

typedef struct bin_manager
{
    struct timeval start_ts;
    struct timeval end_ts;

    metric_index_e metrics[METRICS_COUNT];
    int metrics_count;

    double *bins[METRICS_COUNT];
    int bin_size; // in miliseconds
    long total_bins;
}bin_manager_t;

extern const metric_fn METRIC_REGISTRY[METRICS_COUNT];

typedef enum
{
    BIN_MANAGER_SUCCESS = 0,
    BIN_MANAGER_MALLOC_ERROR = -1,
    BIN_MANAGER_PARAMS_ERROR = -2,
    BIN_MANAGER_OUT_OF_BOUNDS_METRIC_INDEX_ERROR = -3,
}bin_manager_ret_e;

/**
 * @brief The function initializes the bin manager by calculating the total number of bins based on the provided start and end timestamps and the bin size. It also allocates memory for each metric's bins.
 *
 * @param mgr a pointer to the bin_manager_t structure that needs to be initialized.
 *        The structure should have its start_ts, end_ts, metrics, metrics_count, and bin_size fields properly set before calling this function.
 * @return bin_manager_ret_e returns BIN_MANAGER_SUCCESS on successful initialization, BIN_MANAGER_MALLOC_ERROR if memory allocation fails for any of the bins, and BIN_MANAGER_PARAMS_ERROR if the input parameter is invalid (e.g., NULL pointer).
 */
bin_manager_ret_e bin_manager_init(bin_manager_t * mgr);

/**
 * @brief The function processes a packet by determining the appropriate time bin based on the packet's timestamp and updating the corresponding metric values in that bin. It uses the metrics specified in the bin manager to determine which metrics to update for the given packet.
 *
 * @param mgr a pointer to the bin manager to add the packet to its bins
 * @param pkt the packet to be added to the bins
 *
 * @return bin_manager_ret_e returns BIN_MANAGER_SUCCESS on successful initialization, BIN_MANAGER_MALLOC_ERROR if memory allocation fails for any of the bins, and BIN_MANAGER_PARAMS_ERROR if the input parameter is invalid (e.g., NULL pointer).
 */
bin_manager_ret_e bin_manager_process_packet(bin_manager_t *mgr, const packet_info_t *pkt);

/**
 * @brief The function frees only the bins arrays. (useful to change the time bin val).
 *
 * @param mgr a pointer to the bin manager.
 */
void bin_manager_free_bins(bin_manager_t *mgr);

/**
 * @brief The function frees the whole bin manager.
 *
 * @param mgr a pointer to the bin manager.
 */
void bin_manager_free(bin_manager_t *mgr);

#endif