#include "time_series.h"

static metric_index_e all_metrics_static[] = {METRIC_PACKET_COUNT, METRIC_BYTE_COUNT, METRIC_SYN_FLAG_COUNT, METRIC_FIN_FLAG_COUNT, METRIC_RST_FLAG_COUNT, METRIC_ACK_FLAG_COUNT, METRIC_PSH_FLAG_COUNT};

const metric_fn METRIC_REGISTRY[METRICS_COUNT] =
{
    metric_packet_count,   // Index 0
    metric_byte_count,     // Index 1
    metric_syn_flag_count, // Index 2
    metric_fin_flag_count, // Index 3
    metric_rst_flag_count,  // Index 4
    metric_ack_flag_count,  // Index 5
    metric_psh_flag_count   // Index 6
};

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
    /* Use `long` (>= 32 bits, 64 bits on Linux x86_64) for the difference so
     * captures longer than ~24 days don't overflow before we multiply by 1000.
     * `tv_sec` is `time_t` on glibc x86_64 — already 64 bit — so the subtraction
     * is safe; storing into `int` (the previous code) truncated to 32 bits and
     * would silently corrupt the bin index for very long captures.            */
    long seconds_diff = (long)pkt_ts.tv_sec - (long)start_ts.tv_sec;
    long microseconds_diff = (long)pkt_ts.tv_usec - (long)start_ts.tv_usec;
    double total_duration_ms = (double)seconds_diff * MILLISECONDS_IN_SECOND
                             + (double)microseconds_diff / MICROSECONDS_IN_MILLISECOND;
    if (bin_size <= 0)
    {
        /* Guard against a corrupted bin_size — caller already defaults this to
         * DEFAULT_BIN_SIZE in bin_manager_init, so this is just belt+braces.   */
        return 0;
    }
    return (long)(total_duration_ms / bin_size);
}

bin_manager_ret_e bin_manager_init(bin_manager_t * mgr)
{
    bin_manager_ret_e ret_val = BIN_MANAGER_SUCCESS;

    metric_index_e type;

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
        if(mgr->metrics_count <= 0)
        {
            mgr->metrics_count = METRICS_COUNT;
            for (int i = 0; i < METRICS_COUNT; i++)
            {
                mgr->metrics[i] = all_metrics_static[i];
            }
        }

        for (int i = 0; i < mgr->metrics_count && ret_val == BIN_MANAGER_SUCCESS; i++)
        {
            type = mgr->metrics[i];

            if(mgr->bins[type] == NULL)
            {
                mgr->bins[type] = (double *)calloc(mgr->total_bins, sizeof(double));
            }
            if (mgr->bins[type] == NULL)
            {
                printf("Error: Memory allocation failed for bins[%d].\n", i);
                ret_val = BIN_MANAGER_MALLOC_ERROR;
            }
        }
    }
    return ret_val;
}

bin_manager_ret_e bin_manager_process_packet(bin_manager_t *mgr, const packet_info_t *pkt)
{
    bin_manager_ret_e ret_val = BIN_MANAGER_SUCCESS;
    long idx;
    metric_index_e type;


    if(!mgr || !pkt || mgr->metrics_count <= 0)
    {
        printf("Error: bin_manager_process_packet received invalid parameters.\n");
        ret_val = BIN_MANAGER_PARAMS_ERROR;
    }
    else
    {
        idx = calculate_bin_index(pkt->cap_info.ts, mgr->start_ts, mgr->bin_size);

        if(idx < 0 || idx >= mgr->total_bins)
        {
            printf("Warning: Packet timestamp is out of the range of the bin manager. Packet timestamp: %ld.%06ld, Bin manager range: %ld.%06ld - %ld.%06ld\n",
                   pkt->cap_info.ts.tv_sec, pkt->cap_info.ts.tv_usec,
                   mgr->start_ts.tv_sec, mgr->start_ts.tv_usec,
                   mgr->end_ts.tv_sec, mgr->end_ts.tv_usec);
            ret_val = BIN_MANAGER_PACKET_TIMESTAMP_OUT_OF_RANGE_WARNING;
        }
        else
        {
            for (int i = 0; i < mgr->metrics_count; i++)
            {
                type = mgr->metrics[i];
                if(type < 0 || type >= METRICS_COUNT)
                {
                    printf("Error: Metric index %d is out of bounds for METRICS_COUNT %d.\n", type, METRICS_COUNT);
                    ret_val = BIN_MANAGER_OUT_OF_BOUNDS_METRIC_INDEX_ERROR;
                }
                else
                {
                    METRIC_REGISTRY[type](pkt, &(mgr->bins[type][idx]));
                }
            }
        }
    }
    return ret_val;
}

void bin_manager_free_bins(bin_manager_t *mgr)
{
    metric_index_e type;

    if (mgr)
    {
        for (int i = 0; i < mgr->metrics_count; i++)
        {
            type = mgr->metrics[i];
            free(mgr->bins[type]);
            mgr->bins[type] = NULL;
        }
    }
}

void bin_manager_free(bin_manager_t *mgr)
{
    bin_manager_free_bins(mgr);
    free(mgr);
}

bin_manager_ret_e bin_manager_process_packet_block(bin_manager_t *mgr, const packet_block_t *block)
{
    bin_manager_ret_e ret_val = BIN_MANAGER_SUCCESS;

    if (!mgr || !block)
    {
        printf("Error: bin_manager_process_packet_block received NULL pointer.\n");
        ret_val = BIN_MANAGER_PARAMS_ERROR;
    }
    else
    {
        for (uint32_t i = 0; i < block->packet_count && ret_val == BIN_MANAGER_SUCCESS; i++)
        {
            ret_val = bin_manager_process_packet(mgr, &block->packets[i]);
        }
    }
    return ret_val;
}