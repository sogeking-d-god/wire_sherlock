#include "time_series.h"

/**
 * @brief A helper function to calculate the appropriate bin index for a given packet timestamp based on the start timestamp and bin size.
 *
 * @param pkt_ts timestamp of the packet
 * @param start_ts start timestamp of the bin manager
 * @param bin_size size of each bin in milliseconds
 * @return long the calculated bin index for the packet timestamp
 */
static long calculate_bin_index(struct timeval pkt_ts, struct timeval start_ts, double bin_size)
{
    int seconds_diff = pkt_ts.tv_sec - start_ts.tv_sec;
    int microseconds_diff = pkt_ts.tv_usec - start_ts.tv_usec;
    long total_duration = seconds_diff * MILLISECONDS_IN_SECOND + (double)microseconds_diff / MICROSECONDS_IN_MILLISECOND;
    return (long)(total_duration / bin_size);
}

bin_manager_ret_e bin_manager_init(bin_manager_t * mgr)
{
    bin_manager_ret_e ret_val = BIN_MANAGER_SUCCESS;

    if (!mgr)
    {
        printf("Error: bin_manager_init received NULL pointer.\n");
        ret_val = BIN_MANAGER_PARAMS_ERROR;
    }
    else
    {
        // If bin size is not set or invalid, use the default bin size
        if(mgr->bin_size <= 0)
        {
            mgr->bin_size = DEFAULT_BIN_SIZE;
        }

        mgr->total_bins = calculate_bin_index(mgr->end_ts, mgr->start_ts, mgr->bin_size) + 1;

        // If metrics are not set, use all metrics by default
        if(mgr->metrics_count <= 0 && !mgr->metrics)
        {
            mgr->metrics_count = METRICS_COUNT;
            mgr->metrics = ALL_METRICS;
        }

        else if (mgr->metrics_count > 0 || mgr->metrics)
        {
            printf("error: bin_manager_init received invalid metrics configuration. (metrics_count and metrics array conflicting)\n");
            ret_val = BIN_MANAGER_PARAMS_ERROR;
        }

        for (int i = 0; i < mgr->metrics_count && ret_val == BIN_MANAGER_SUCCESS; i++)
        {
            if(mgr->bins[i] == NULL)
            {
                mgr->bins[i] = (double *)calloc(mgr->total_bins, sizeof(double));
            }
            if (mgr->bins[i] == NULL)
            {
                printf("Error: Memory allocation failed for bins[%d].\n", i);
                ret_val = BIN_MANAGER_MALLOC_ERROR;
            }
        }
        return ret_val;
    }
}

bin_manager_ret_e bin_manager_process_packet(bin_manager_t *mgr, const packet_info_t *pkt)
{
    bin_manager_ret_e ret_val = BIN_MANAGER_SUCCESS;
    long idx;
    if(!mgr || !pkt || mgr->metrics_count <= 0 || !mgr->metrics)
    {
        printf("Error: bin_manager_process_packet received invalid parameters.\n");
        ret_val = BIN_MANAGER_PARAMS_ERROR;
    }
    else
    {
        idx = calculate_bin_index(pkt->cap_info.ts, mgr->start_ts, mgr->bin_size);
        for (int i = 0; i < mgr->metrics_count; i++)
        {
            if(mgr->metrics[i] < 0 || mgr->metrics[i] >= METRICS_COUNT)
            {
                printf("Error: Metric index %d is out of bounds for METRICS_COUNT %d.\n", mgr->metrics[i], METRICS_COUNT);
                ret_val = BIN_MANAGER_OUT_OF_BOUNDS_METRIC_INDEX_ERROR;
            }
            else
            {
                METRIC_REGISTRY[mgr->metrics[i]](pkt, &(mgr->bins[i][idx]));
            }
        }
    }
    return ret_val;
}

void bin_manager_free_bins(bin_manager_t *mgr)
{
    for (int i = 0; i < mgr->metrics_count; i++)
    {
        free(mgr->bins[i]);
        mgr->bins[i] = NULL;
    }
}

void bin_manager_free(bin_manager_t *mgr)
{
    bin_manager_free_bins(mgr);
    free(mgr->metrics);
    mgr->metrics = NULL;
}