#include <math.h>
#include <stdlib.h>
#include "micro_detector.h"
#include "dbscan.h"
#include "point.h"

/**
 * @brief Returns the absolute value of a double without pulling extra dependencies.
 *
 * @param value Input value.
 * @return double |value|.
 */
static double micro_abs(double value)
{
    double ret_val;

    ret_val = (value < 0.0) ? -value : value;
    return ret_val;
}

/**
 * @brief Appends one event to a dynamically-grown buffer, doubling capacity on demand.
 *
 * @param items Pointer to the items array pointer (may be reallocated).
 * @param count Pointer to the current element count (incremented on success).
 * @param capacity Pointer to the current allocated capacity (updated on growth).
 * @param new_event Event to append.
 * @return int 1 on success, 0 on allocation failure (caller frees and aborts).
 */
static int micro_events_buffer_push(micro_event_t **items, int *count, int *capacity, const micro_event_t *new_event)
{
    int ret_val;
    int new_capacity;
    micro_event_t *grown;

    ret_val = 0;

    if (*count >= *capacity)
    {
        new_capacity = (*capacity == 0) ? 1 : (*capacity * 2);
        grown = (micro_event_t *)realloc(*items, (size_t)new_capacity * sizeof(micro_event_t));
        if (grown != NULL)
        {
            *items = grown;
            *capacity = new_capacity;
        }
    }

    if (*count < *capacity)
    {
        (*items)[*count] = *new_event;
        *count = *count + 1;
        ret_val = 1;
    }

    return ret_val;
}

/**
 * @brief Scans one metric's bin array with a streaming trailing EWMA mean and
 *        EWMA variance, emitting a micro_event_t per bin whose |Z_sliding| exceeds
 *        the configured threshold.
 *
 * @param bin_values Pointer to the metric's bin array (length total_bins).
 * @param total_bins Number of bins in the array.
 * @param metric Metric tag stored into each emitted event.
 * @param alpha EWMA smoothing factor (typically in (0, 1)).
 * @param z_threshold Absolute sliding-Z cutoff for event emission.
 * @param out_items Pointer to the (possibly growing) events buffer pointer.
 * @param out_count Pointer to the events count (incremented).
 * @param out_capacity Pointer to the events allocated capacity (grown as needed).
 * @return int 1 on success (including zero events), 0 if a buffer growth failed.
 */
static int micro_sweep_one_metric(const double *bin_values, long total_bins, metric_index_e metric, double alpha, double z_threshold,
                                  micro_event_t **out_items, int *out_count, int *out_capacity)
{
    int ret_val;
    long i;
    int proceed;
    double ewma_mean_prev;
    double ewma_var_prev;
    double ewma_mean_next;
    double ewma_var_next;
    double delta;
    double sigma;
    double z_value;
    double abs_z;
    double one_minus_alpha;
    micro_event_t candidate;

    ret_val = 1;
    proceed = 1;

    if (bin_values != NULL && total_bins > 0)
    {
        ewma_mean_prev = bin_values[0];
        ewma_var_prev = 0.0;
        one_minus_alpha = MICRO_ONE_MINUS_DEFAULT - alpha;

        i = 1;
        while (i < total_bins && proceed == 1)
        {
            delta = bin_values[i] - ewma_mean_prev;
            ewma_mean_next = ewma_mean_prev + alpha * delta;
            // EWMA variance: leans on prior variance plus weighted squared delta.
            ewma_var_next = one_minus_alpha * (ewma_var_prev + alpha * delta * delta);

            if ((long)i >= (long)MICRO_EWMA_WARMUP_BINS && ewma_var_next > MICRO_EWMA_VAR_FLOOR)
            {
                sigma = sqrt(ewma_var_next);
                // Baseline against PRIOR mean so the spike at i does not pollute its own reference.
                z_value = (bin_values[i] - ewma_mean_prev) / sigma;
                abs_z = micro_abs(z_value);

                if (abs_z > z_threshold)
                {
                    candidate.metric = metric;
                    candidate.bin_index = (uint64_t)i;
                    candidate.value = bin_values[i];
                    candidate.z_sliding = z_value;

                    if (micro_events_buffer_push(out_items, out_count, out_capacity, &candidate) == 0)
                    {
                        ret_val = 0;
                        proceed = 0;
                    }
                }
            }

            ewma_mean_prev = ewma_mean_next;
            ewma_var_prev = ewma_var_next;
            i = i + 1;
        }
    }

    return ret_val;
}

/**
 * @brief Projects micro events onto a true 1D temporal point set.
 *        Sets dim_count = MICRO_EVENT_DIM_COUNT (1) so the KD-tree builds and
 *        searches over a single axis (bin_index). Padding higher dimensions
 *        with zeros while leaving dim_count = 3 would have caused the tree
 *        to split on constant-zero axes at depths 1 and 2, degrading queries.
 *
 * @param events Source events list.
 * @param out_points Output points; arr is malloc'd, caller frees via micro_points_free.
 * @return int 1 on success, 0 on allocation failure.
 */
static int micro_build_event_points(const micro_events_list_t *events, points_arr_t *out_points)
{
    int ret_val;
    int i;
    point_t *buffer;

    ret_val = 0;

    if (events != NULL && out_points != NULL && events->count > 0)
    {
        buffer = (point_t *)calloc((size_t)events->count, sizeof(point_t));
        if (buffer != NULL)
        {
            for (i = 0; i < events->count; i++)
            {
                buffer[i].vals[0] = (double)events->items[i].bin_index;
                buffer[i].original_index = i;
                buffer[i].cluster_id = UNCLASSIFIED;
            }

            out_points->arr = buffer;
            out_points->len = events->count;
            out_points->dim_count = MICRO_EVENT_DIM_COUNT;
            ret_val = 1;
        }
    }

    return ret_val;
}

/**
 * @brief Frees the arr buffer of a points_arr_t built by micro_build_event_points.
 *
 * @param points Container whose internal array is freed and pointer cleared.
 */
static void micro_points_free(points_arr_t *points)
{
    if (points != NULL)
    {
        if (points->arr != NULL)
        {
            free(points->arr);
            points->arr = NULL;
        }
        points->len = 0;
        points->dim_count = 0;
    }
}

/**
 * @brief Counts the number of distinct cluster ids assigned by DBSCAN (ignoring NOISE
 *        and UNCLASSIFIED). DBSCAN tags clusters as FIRST_CLUSTER (1) and upward.
 *
 * @param points Tagged point set (post-DBSCAN).
 * @return int Maximum positive cluster_id found, or 0 if no clusters exist.
 */
static int micro_count_clusters(const points_arr_t *points)
{
    int ret_val;
    int i;
    int current_id;

    ret_val = 0;

    if (points != NULL && points->arr != NULL)
    {
        for (i = 0; i < points->len; i++)
        {
            current_id = points->arr[i].cluster_id;
            if (current_id > ret_val)
            {
                ret_val = current_id;
            }
        }
    }

    return ret_val;
}

/**
 * @brief Materializes one micro_burst_t per DBSCAN cluster id. For each cluster the
 *        function records its member event indexes, the temporal span [bin_start, bin_end],
 *        and the cluster id itself.
 *
 * @param points Tagged DBSCAN point set; original_index references events->items.
 * @param events Source events list referenced by member indexes.
 * @param cluster_count Number of distinct positive cluster ids in points.
 * @param out_bursts Output bursts list; items malloc'd on success.
 * @return int 1 on success, 0 on allocation failure.
 */
static int micro_build_bursts(const points_arr_t *points, const micro_events_list_t *events, int cluster_count, micro_bursts_list_t *out_bursts)
{
    int ret_val;
    int point_idx;
    int event_idx;
    int target_cluster;
    int slot_within_cluster;
    int allocation_ok;
    int cluster_iter;
    int total_size;
    int *write_cursor;
    micro_burst_t *bursts_buffer;
    uint64_t bin_value;

    ret_val = 0;
    bursts_buffer = NULL;
    write_cursor = NULL;
    allocation_ok = 1;

    if (out_bursts != NULL && points != NULL && events != NULL && cluster_count > 0)
    {
        bursts_buffer = (micro_burst_t *)calloc((size_t)cluster_count, sizeof(micro_burst_t));
        write_cursor = (int *)calloc((size_t)cluster_count, sizeof(int));

        if (bursts_buffer != NULL && write_cursor != NULL)
        {
            // Pass 1: count members per cluster
            for (point_idx = 0; point_idx < points->len; point_idx++)
            {
                target_cluster = points->arr[point_idx].cluster_id;
                if (target_cluster >= FIRST_CLUSTER && target_cluster <= cluster_count)
                {
                    total_size = bursts_buffer[target_cluster - FIRST_CLUSTER].member_count + 1;
                    bursts_buffer[target_cluster - FIRST_CLUSTER].member_count = total_size;
                }
            }

            // Pass 2: allocate each cluster's member array and seed span sentinels.
            cluster_iter = 0;
            while (cluster_iter < cluster_count && allocation_ok == 1)
            {
                total_size = bursts_buffer[cluster_iter].member_count;
                if (total_size > 0)
                {
                    bursts_buffer[cluster_iter].member_indexes =
                        (int *)calloc((size_t)total_size, sizeof(int));
                    if (bursts_buffer[cluster_iter].member_indexes == NULL)
                    {
                        allocation_ok = 0;
                    }
                }
                bursts_buffer[cluster_iter].cluster_id = cluster_iter + FIRST_CLUSTER;
                bursts_buffer[cluster_iter].bin_start = UINT64_MAX;
                bursts_buffer[cluster_iter].bin_end = 0;
                cluster_iter = cluster_iter + 1;
            }

            if (allocation_ok == 1)
            {
                // Pass 3: fill member arrays via a separate cursor, and compute [bin_start, bin_end].
                for (point_idx = 0; point_idx < points->len; point_idx++)
                {
                    target_cluster = points->arr[point_idx].cluster_id;
                    if (target_cluster >= FIRST_CLUSTER && target_cluster <= cluster_count)
                    {
                        event_idx = points->arr[point_idx].original_index;
                        if (event_idx >= 0 && event_idx < events->count)
                        {
                            slot_within_cluster = write_cursor[target_cluster - FIRST_CLUSTER];
                            bursts_buffer[target_cluster - FIRST_CLUSTER].member_indexes[slot_within_cluster] = event_idx;
                            write_cursor[target_cluster - FIRST_CLUSTER] = slot_within_cluster + 1;

                            bin_value = events->items[event_idx].bin_index;
                            if (bin_value < bursts_buffer[target_cluster - FIRST_CLUSTER].bin_start)
                            {
                                bursts_buffer[target_cluster - FIRST_CLUSTER].bin_start = bin_value;
                            }
                            if (bin_value > bursts_buffer[target_cluster - FIRST_CLUSTER].bin_end)
                            {
                                bursts_buffer[target_cluster - FIRST_CLUSTER].bin_end = bin_value;
                            }
                        }
                    }
                }

                out_bursts->items = bursts_buffer;
                out_bursts->count = cluster_count;
                ret_val = 1;
            }
            else
            {

                for (cluster_iter = 0; cluster_iter < cluster_count; cluster_iter++)
                {
                    free(bursts_buffer[cluster_iter].member_indexes);
                }
                free(bursts_buffer);
            }
        }
        else
        {
            if (bursts_buffer != NULL)
            {
                free(bursts_buffer);
            }
            fprintf(stderr, "Allocation failure in micro_build_bursts.\n");
        }

        if (write_cursor != NULL)
        {
            free(write_cursor);
        }
    }

    return ret_val;
}

void micro_events_list_free(micro_events_list_t *events)
{
    if (events != NULL)
    {
        if (events->items != NULL)
        {
            free(events->items);
            events->items = NULL;
        }
        events->count = 0;
    }
}

void micro_bursts_list_free(micro_bursts_list_t *bursts)
{
    int i;

    if (bursts != NULL)
    {
        if (bursts->items != NULL)
        {
            for (i = 0; i < bursts->count; i++)
            {
                if (bursts->items[i].member_indexes != NULL)
                {
                    free(bursts->items[i].member_indexes);
                    bursts->items[i].member_indexes = NULL;
                }
            }
            free(bursts->items);
            bursts->items = NULL;
        }
        bursts->count = 0;
    }
}

int micro_detector_run(const bin_manager_t *bins, const anomaly_config_t *cfg, micro_events_list_t *out_events, micro_bursts_list_t *out_bursts)
{
    int ret_val;
    int proceed;
    int metric_iter;
    int sweep_ok;
    int cluster_count;
    int dbscan_result;
    double eps_used;
    micro_event_t *events_buffer;
    int events_count;
    int events_capacity;
    metric_index_e current_metric;
    points_arr_t event_points;

    ret_val = 0;
    proceed = 1;
    events_buffer = NULL;
    events_count = 0;
    events_capacity = 0;
    event_points.arr = NULL;
    event_points.len = 0;
    event_points.dim_count = 0;

    if (bins != NULL && cfg != NULL && out_events != NULL && out_bursts != NULL)
    {
        out_events->items = NULL;
        out_events->count = 0;
        out_bursts->items = NULL;
        out_bursts->count = 0;

        // Phase 1: streaming EWMA sweep per active metric, accumulating events.
        metric_iter = 0;
        while (metric_iter < bins->metrics_count && proceed == 1)
        {
            current_metric = bins->metrics[metric_iter];
            if ((int)current_metric >= 0 && (int)current_metric < METRICS_COUNT)
            {
                sweep_ok = micro_sweep_one_metric(bins->bins[current_metric], bins->total_bins, current_metric, cfg->ewma_alpha,
                                                  cfg->micro_z_threshold, &events_buffer, &events_count, &events_capacity);
                if (sweep_ok == 0)
                {
                    proceed = 0;
                }
            }
            metric_iter = metric_iter + 1;
        }

        if (proceed == 1)
        {
            out_events->items = events_buffer;
            out_events->count = events_count;

            // Phase 2: cluster events temporally with the existing 3D DBSCAN as 1D.
            if (events_count >= cfg->micro_min_pts)
            {
                if (micro_build_event_points(out_events, &event_points) == 1)
                {
                    dbscan_result = dbscan_process_auto(&event_points, cfg->micro_min_pts, &eps_used);
                    if (dbscan_result >= 0)
                    {
                        cluster_count = micro_count_clusters(&event_points);
                        if (cluster_count > 0)
                        {
                            if (micro_build_bursts(&event_points, out_events, cluster_count, out_bursts) == 1)
                            {
                                ret_val = 1;
                            }
                        }
                        else
                        {
                            // DBSCAN found only noise; success with zero bursts.
                            ret_val = 1;
                        }
                    }
                    micro_points_free(&event_points);
                }
            }
            else
            {
                // Too few events to cluster; success with zero bursts.
                ret_val = 1;
            }
        }
        else
        {
            // Allocation failure during sweep; release partial events buffer.
            if (events_buffer != NULL)
            {
                free(events_buffer);
            }
            out_events->items = NULL;
            out_events->count = 0;
        }
    }

    return ret_val;
}
