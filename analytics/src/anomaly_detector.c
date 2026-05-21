#include <stdlib.h>
#include <math.h>
#include "anomaly_detector.h"
#include "stats.h"
#include "normalize.h"
#include "micro_detector.h"
#include "dbscan.h"
#include "point.h"

/**
 * @brief Returns 1 if the given metric is selected by the mask, 0 otherwise.
 *        A metric is selected when (mask >> metric) & 1 == 1.
 *
 * @param mask Bitmask of metric_index_e values.
 * @param metric Metric to test.
 * @return int 1 if selected, 0 otherwise.
 */
static int anomaly_metric_selected(uint32_t mask, metric_index_e metric)
{
    int ret_val;
    uint32_t bit;

    ret_val = 0;
    if ((int)metric >= 0 && (int)metric < METRICS_COUNT)
    {
        bit = 1U << ((unsigned int)metric);
        if ((mask & bit) != 0U)
        {
            ret_val = 1;
        }
    }
    return ret_val;
}

/**
 * @brief Grows a scored_segments_list_t to hold at least required_capacity items.
 *        The function only allocates; it never shrinks. On failure the original
 *        buffer remains valid.
 *
 * @param segments List to grow.
 * @param required_capacity Minimum number of items the buffer must hold after the call.
 * @return int 1 on success or when no growth is required, 0 on allocation failure.
 */
static int anomaly_scored_segments_reserve(scored_segments_list_t *segments, int required_capacity)
{
    int ret_val;
    scored_segment_t *grown;

    ret_val = 0;

    if (segments != NULL && required_capacity > 0)
    {
        grown = (scored_segment_t *)realloc(segments->items,
                                            (size_t)required_capacity * sizeof(scored_segment_t));
        if (grown != NULL)
        {
            segments->items = grown;
            ret_val = 1;
        }
    }
    else if (segments != NULL)
    {
        ret_val = 1;
    }

    return ret_val;
}

/**
 * @brief Rebuilds a prefix_sums_t over a raw bin array. Needed because PELT frees
 *        its internal prefix sums before returning. Computing them again is O(n)
 *        and avoids changing the public PELT signature.
 *
 * @param bin_values Source bin array.
 * @param bin_count Number of elements in bin_values.
 * @param out_ps Output prefix sums (caller frees sum and sum_sq on success).
 * @return int 1 on success, 0 on allocation failure.
 */
static int anomaly_build_prefix_sums(const double *bin_values, long bin_count, prefix_sums_t *out_ps)
{
    int ret_val;
    long i;

    ret_val = 0;

    if (bin_values != NULL && bin_count > 0 && out_ps != NULL)
    {
        out_ps->sum = (double *)calloc((size_t)(bin_count + 1), sizeof(double));
        out_ps->sum_sq = (double *)calloc((size_t)(bin_count + 1), sizeof(double));

        if (out_ps->sum != NULL && out_ps->sum_sq != NULL)
        {
            for (i = 0; i < bin_count; i++)
            {
                out_ps->sum[i + 1] = out_ps->sum[i] + bin_values[i];
                out_ps->sum_sq[i + 1] = out_ps->sum_sq[i] + bin_values[i] * bin_values[i];
            }
            ret_val = 1;
        }
        else
        {
            if (out_ps->sum != NULL)
            {
                free(out_ps->sum);
                out_ps->sum = NULL;
            }
            if (out_ps->sum_sq != NULL)
            {
                free(out_ps->sum_sq);
                out_ps->sum_sq = NULL;
            }
        }
    }

    return ret_val;
}

/**
 * @brief Releases prefix sums allocated by anomaly_build_prefix_sums.
 *
 * @param ps Prefix sums to release; pointers cleared.
 */
static void anomaly_free_prefix_sums(prefix_sums_t *ps)
{
    if (ps != NULL)
    {
        if (ps->sum != NULL)
        {
            free(ps->sum);
            ps->sum = NULL;
        }
        if (ps->sum_sq != NULL)
        {
            free(ps->sum_sq);
            ps->sum_sq = NULL;
        }
    }
}

/**
 * @brief Runs PELT + Global Z + SSMD scoring for a SINGLE metric, then appends
 *        the resulting segments (tagged with metric) into out_segments. Used as
 *        the per-metric inner loop of anomaly_generate_macro_segments.
 *
 * @param bin_values Bin array for this metric.
 * @param bin_count Length of bin_values.
 * @param metric Tag stored on every appended segment.
 * @param z_sensitivity Threshold passed to stats_analyze_with_standard_error.
 * @param out_segments Append-only output list.
 * @return int 1 on success, 0 on allocation failure.
 */
static int anomaly_process_one_metric(const double *bin_values,
                                      long bin_count,
                                      metric_index_e metric,
                                      double z_sensitivity,
                                      scored_segments_list_t *out_segments)
{
    int ret_val;
    int reservation_ok;
    int prefix_ok;
    int i;
    double penalty;
    time_series_t ts_view;
    pelt_segments_list_t pelt_result;
    prefix_sums_t local_ps;
    scored_segment_t *target;
    int new_total_count;

    ret_val = 0;
    pelt_result.segments = NULL;
    pelt_result.count = 0;
    local_ps.sum = NULL;
    local_ps.sum_sq = NULL;

    if (bin_values != NULL && bin_count > 0 && out_segments != NULL)
    {
        ts_view.data = (double *)bin_values;
        ts_view.count = (uint64_t)bin_count;

        penalty = pelt_calculate_penalty_BIC((uint64_t)bin_count, ANOMALY_PELT_PARAM_COUNT);
        pelt_result = pelt_detect_changepoints(&ts_view, penalty);

        if (pelt_result.segments != NULL && pelt_result.count > 0)
        {
            prefix_ok = anomaly_build_prefix_sums(bin_values, bin_count, &local_ps);
            if (prefix_ok == 1)
            {
                stats_analyze_with_standard_error(&pelt_result,
                                                  &local_ps,
                                                  (uint64_t)bin_count,
                                                  z_sensitivity);
                stats_compute_ssmd_against_prev(&pelt_result);

                new_total_count = out_segments->count + pelt_result.count;
                reservation_ok = anomaly_scored_segments_reserve(out_segments, new_total_count);

                if (reservation_ok == 1)
                {
                    for (i = 0; i < pelt_result.count; i++)
                    {
                        target = &out_segments->items[out_segments->count + i];
                        target->metric = metric;
                        target->start_bin = pelt_result.segments[i].start;
                        target->end_bin = pelt_result.segments[i].end;
                        target->mean = pelt_result.segments[i].mean;
                        target->variance = pelt_result.segments[i].variance;
                        target->ssmd = pelt_result.segments[i].ssmd;
                        target->z_global = pelt_result.segments[i].z_score;
                    }
                    out_segments->count = new_total_count;
                    ret_val = 1;
                }
            }
            anomaly_free_prefix_sums(&local_ps);
        }
        else if (pelt_result.count == 0)
        {
            // PELT produced zero segments (degenerate input); not an allocation failure.
            ret_val = 1;
        }

        if (pelt_result.segments != NULL)
        {
            free(pelt_result.segments);
        }
    }

    return ret_val;
}

/**
 * @brief Counts how many segments in the list match the supplied mask.
 *
 * @param segments Source list.
 * @param mask Metric bitmask.
 * @return int Number of selected segments (0 if list is NULL or empty).
 */
static int anomaly_count_selected_segments(const scored_segments_list_t *segments, uint32_t mask)
{
    int ret_val;
    int i;

    ret_val = 0;

    if (segments != NULL && segments->items != NULL)
    {
        for (i = 0; i < segments->count; i++)
        {
            if (anomaly_metric_selected(mask, segments->items[i].metric) == 1)
            {
                ret_val = ret_val + 1;
            }
        }
    }

    return ret_val;
}

/**
 * @brief Builds the 3D normalized Macro point set from the subset of segments
 *        selected by mask. point.original_index references the FULL segments list.
 *
 * @param segments Full segments list.
 * @param mask Metric bitmask.
 * @param bin_size_ms Bin width in milliseconds.
 * @param pcap_duration_seconds PCAP duration.
 * @param cfg Configuration (axis weights, saturation knees).
 * @param out_points Output points (caller frees arr).
 * @return int 1 on success, 0 on allocation failure or empty selection.
 */
static int anomaly_build_filtered_macro_points(const scored_segments_list_t *segments,
                                               uint32_t mask,
                                               int bin_size_ms,
                                               double pcap_duration_seconds,
                                               const anomaly_config_t *cfg,
                                               points_arr_t *out_points)
{
    int ret_val;
    int selected_count;
    int i;
    int write_slot;
    point_t *buffer;

    ret_val = 0;
    buffer = NULL;

    if (segments != NULL && cfg != NULL && out_points != NULL)
    {
        selected_count = anomaly_count_selected_segments(segments, mask);
        if (selected_count > 0)
        {
            buffer = (point_t *)calloc((size_t)selected_count, sizeof(point_t));
            if (buffer != NULL)
            {
                write_slot = 0;
                for (i = 0; i < segments->count; i++)
                {
                    if (anomaly_metric_selected(mask, segments->items[i].metric) == 1)
                    {
                        normalize_segment_to_point(&segments->items[i],
                                                   i,
                                                   bin_size_ms,
                                                   pcap_duration_seconds,
                                                   cfg,
                                                   &buffer[write_slot]);
                        write_slot = write_slot + 1;
                    }
                }

                out_points->arr = buffer;
                out_points->len = selected_count;
                out_points->dim_count = MAX_DIM_COUNT;
                ret_val = 1;
            }
        }
    }

    return ret_val;
}

/**
 * @brief Counts the maximum positive cluster id assigned by DBSCAN over the point set.
 *
 * @param points Tagged point set.
 * @return int Number of clusters (0 if all points are NOISE/UNCLASSIFIED).
 */
static int anomaly_count_clusters(const points_arr_t *points)
{
    int ret_val;
    int i;

    ret_val = 0;

    if (points != NULL && points->arr != NULL)
    {
        for (i = 0; i < points->len; i++)
        {
            if (points->arr[i].cluster_id > ret_val)
            {
                ret_val = points->arr[i].cluster_id;
            }
        }
    }

    return ret_val;
}

/**
 * @brief Materializes macro_cluster_t entries from DBSCAN-tagged points.
 *        Each cluster records its member segment indexes (relative to the FULL
 *        segments list, not the filtered point subset), temporal span, and the
 *        maximum |SSMD| / |Z_global| within the cluster (for downstream LLM use).
 *
 * @param points Tagged point set (original_index references full segments list).
 * @param segments Full segments list for metadata lookups.
 * @param cluster_count Number of positive cluster ids.
 * @param out_clusters Output cluster list (caller frees via anomaly_macro_clusters_free).
 * @return int 1 on success, 0 on allocation failure.
 */
static int anomaly_build_macro_clusters(const points_arr_t *points,
                                        const scored_segments_list_t *segments,
                                        int cluster_count,
                                        macro_clusters_list_t *out_clusters)
{
    int ret_val;
    int point_idx;
    int seg_idx;
    int target_cluster;
    int slot;
    int allocation_ok;
    int cluster_iter;
    int total_size;
    int *write_cursor;
    macro_cluster_t *clusters_buffer;
    double abs_ssmd;
    double abs_z;
    uint64_t start_bin;
    uint64_t end_bin;

    ret_val = 0;
    clusters_buffer = NULL;
    write_cursor = NULL;
    allocation_ok = 1;

    if (out_clusters != NULL && points != NULL && segments != NULL && cluster_count > 0)
    {
        clusters_buffer = (macro_cluster_t *)calloc((size_t)cluster_count, sizeof(macro_cluster_t));
        write_cursor = (int *)calloc((size_t)cluster_count, sizeof(int));

        if (clusters_buffer != NULL && write_cursor != NULL)
        {
            // Pass 1: tally member counts per cluster id.
            for (point_idx = 0; point_idx < points->len; point_idx++)
            {
                target_cluster = points->arr[point_idx].cluster_id;
                if (target_cluster >= FIRST_CLUSTER && target_cluster <= cluster_count)
                {
                    total_size = clusters_buffer[target_cluster - FIRST_CLUSTER].member_count + 1;
                    clusters_buffer[target_cluster - FIRST_CLUSTER].member_count = total_size;
                }
            }

            // Pass 2: per-cluster member arrays and span/aggregate sentinels.
            cluster_iter = 0;
            while (cluster_iter < cluster_count && allocation_ok == 1)
            {
                total_size = clusters_buffer[cluster_iter].member_count;
                if (total_size > 0)
                {
                    clusters_buffer[cluster_iter].member_indexes =
                        (int *)calloc((size_t)total_size, sizeof(int));
                    if (clusters_buffer[cluster_iter].member_indexes == NULL)
                    {
                        allocation_ok = 0;
                    }
                }
                clusters_buffer[cluster_iter].cluster_id = cluster_iter + FIRST_CLUSTER;
                clusters_buffer[cluster_iter].time_start_bin = UINT64_MAX;
                clusters_buffer[cluster_iter].time_end_bin = 0;
                clusters_buffer[cluster_iter].max_abs_ssmd = 0.0;
                clusters_buffer[cluster_iter].max_abs_z = 0.0;
                cluster_iter = cluster_iter + 1;
            }

            if (allocation_ok == 1)
            {
                // Pass 3: fill member arrays, compute span and per-cluster maxima.
                for (point_idx = 0; point_idx < points->len; point_idx++)
                {
                    target_cluster = points->arr[point_idx].cluster_id;
                    if (target_cluster >= FIRST_CLUSTER && target_cluster <= cluster_count)
                    {
                        seg_idx = points->arr[point_idx].original_index;
                        if (seg_idx >= 0 && seg_idx < segments->count)
                        {
                            slot = write_cursor[target_cluster - FIRST_CLUSTER];
                            clusters_buffer[target_cluster - FIRST_CLUSTER].member_indexes[slot] = seg_idx;
                            write_cursor[target_cluster - FIRST_CLUSTER] = slot + 1;

                            start_bin = segments->items[seg_idx].start_bin;
                            end_bin = segments->items[seg_idx].end_bin;

                            if (start_bin < clusters_buffer[target_cluster - FIRST_CLUSTER].time_start_bin)
                            {
                                clusters_buffer[target_cluster - FIRST_CLUSTER].time_start_bin = start_bin;
                            }
                            if (end_bin > clusters_buffer[target_cluster - FIRST_CLUSTER].time_end_bin)
                            {
                                clusters_buffer[target_cluster - FIRST_CLUSTER].time_end_bin = end_bin;
                            }

                            abs_ssmd = segments->items[seg_idx].ssmd;
                            if (abs_ssmd < 0.0)
                            {
                                abs_ssmd = -abs_ssmd;
                            }
                            if (abs_ssmd > clusters_buffer[target_cluster - FIRST_CLUSTER].max_abs_ssmd)
                            {
                                clusters_buffer[target_cluster - FIRST_CLUSTER].max_abs_ssmd = abs_ssmd;
                            }

                            abs_z = segments->items[seg_idx].z_global;
                            if (abs_z < 0.0)
                            {
                                abs_z = -abs_z;
                            }
                            if (abs_z > clusters_buffer[target_cluster - FIRST_CLUSTER].max_abs_z)
                            {
                                clusters_buffer[target_cluster - FIRST_CLUSTER].max_abs_z = abs_z;
                            }
                        }
                    }
                }

                out_clusters->items = clusters_buffer;
                out_clusters->count = cluster_count;
                ret_val = 1;
            }
            else
            {
                for (cluster_iter = 0; cluster_iter < cluster_count; cluster_iter++)
                {
                    if (clusters_buffer[cluster_iter].member_indexes != NULL)
                    {
                        free(clusters_buffer[cluster_iter].member_indexes);
                    }
                }
                free(clusters_buffer);
            }
        }
        else
        {
            if (clusters_buffer != NULL)
            {
                free(clusters_buffer);
            }
        }

        if (write_cursor != NULL)
        {
            free(write_cursor);
        }
    }

    return ret_val;
}

/**
 * @brief Reuses micro_detector logic for a SINGLE metric: streaming EWMA sweep
 *        producing events that are appended into out_events. The function is a
 *        thin re-implementation of the per-metric portion of micro_detector_run
 *        so granular Macro/Micro stages can be invoked independently from Python.
 *
 *        We replicate the streaming pass here (rather than calling micro_detector_run)
 *        to honor the metric_mask granularity without invoking on metrics the caller
 *        did not select.
 *
 * @param bin_values Bin array for this metric.
 * @param bin_count Number of bins.
 * @param metric Metric tag stored on each event.
 * @param alpha EWMA smoothing factor.
 * @param z_threshold Absolute sliding-Z cutoff.
 * @param out_events Append-only events list.
 * @return int 1 on success, 0 on allocation failure.
 */
static int anomaly_sweep_micro_for_metric(const double *bin_values,
                                          long bin_count,
                                          metric_index_e metric,
                                          double alpha,
                                          double z_threshold,
                                          micro_events_list_t *out_events)
{
    int ret_val;
    int proceed;
    long i;
    int new_total_count;
    int reservation_ok;
    double ewma_mean_prev;
    double ewma_var_prev;
    double ewma_mean_next;
    double ewma_var_next;
    double one_minus_alpha;
    double delta;
    double sigma;
    double z_value;
    double abs_z;
    micro_event_t *grown;

    ret_val = 1;
    proceed = 1;

    if (bin_values != NULL && bin_count > 0 && out_events != NULL)
    {
        ewma_mean_prev = bin_values[0];
        ewma_var_prev = 0.0;
        one_minus_alpha = MICRO_ONE_MINUS_DEFAULT - alpha;

        i = 1;
        while (i < bin_count && proceed == 1)
        {
            delta = bin_values[i] - ewma_mean_prev;
            ewma_mean_next = ewma_mean_prev + alpha * delta;
            ewma_var_next = one_minus_alpha * (ewma_var_prev + alpha * delta * delta);

            if ((long)i >= (long)MICRO_EWMA_WARMUP_BINS && ewma_var_next > MICRO_EWMA_VAR_FLOOR)
            {
                sigma = sqrt(ewma_var_next);
                // Baseline against PRIOR mean so the spike at i does not pollute its own reference.
                z_value = (bin_values[i] - ewma_mean_prev) / sigma;
                abs_z = (z_value < 0.0) ? -z_value : z_value;

                if (abs_z > z_threshold)
                {
                    new_total_count = out_events->count + 1;
                    grown = (micro_event_t *)realloc(out_events->items,
                                                    (size_t)new_total_count * sizeof(micro_event_t));
                    reservation_ok = (grown != NULL) ? 1 : 0;

                    if (reservation_ok == 1)
                    {
                        out_events->items = grown;
                        out_events->items[out_events->count].metric = metric;
                        out_events->items[out_events->count].bin_index = (uint64_t)i;
                        out_events->items[out_events->count].value = bin_values[i];
                        out_events->items[out_events->count].z_sliding = z_value;
                        out_events->count = new_total_count;
                    }
                    else
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
 * @brief Builds a 1D temporal point set (vals[0] = bin_index) from the subset of
 *        events selected by mask. original_index references the FULL events list.
 *
 * @param events Full events list.
 * @param mask Metric bitmask.
 * @param out_points Output points (caller frees arr).
 * @return int 1 on success, 0 on allocation failure or empty selection.
 */
static int anomaly_build_filtered_micro_points(const micro_events_list_t *events,
                                               uint32_t mask,
                                               points_arr_t *out_points)
{
    int ret_val;
    int i;
    int selected_count;
    int write_slot;
    point_t *buffer;

    ret_val = 0;
    buffer = NULL;

    if (events != NULL && events->items != NULL && out_points != NULL)
    {
        selected_count = 0;
        for (i = 0; i < events->count; i++)
        {
            if (anomaly_metric_selected(mask, events->items[i].metric) == 1)
            {
                selected_count = selected_count + 1;
            }
        }

        if (selected_count > 0)
        {
            buffer = (point_t *)calloc((size_t)selected_count, sizeof(point_t));
            if (buffer != NULL)
            {
                write_slot = 0;
                for (i = 0; i < events->count; i++)
                {
                    if (anomaly_metric_selected(mask, events->items[i].metric) == 1)
                    {
                        buffer[write_slot].vals[0] = (double)events->items[i].bin_index;
                        buffer[write_slot].vals[1] = 0.0;
                        buffer[write_slot].vals[2] = 0.0;
                        buffer[write_slot].original_index = i;
                        buffer[write_slot].cluster_id = UNCLASSIFIED;
                        write_slot = write_slot + 1;
                    }
                }

                out_points->arr = buffer;
                out_points->len = selected_count;
                out_points->dim_count = MAX_DIM_COUNT;
                ret_val = 1;
            }
        }
    }

    return ret_val;
}

/**
 * @brief Materializes micro_burst_t entries from DBSCAN-tagged 1D points.
 *        member_indexes reference the FULL events list.
 *
 * @param points Tagged point set.
 * @param events Full events list.
 * @param cluster_count Number of positive cluster ids.
 * @param out_bursts Output bursts (caller frees via micro_bursts_list_free).
 * @return int 1 on success, 0 on allocation failure.
 */
static int anomaly_build_micro_bursts(const points_arr_t *points,
                                      const micro_events_list_t *events,
                                      int cluster_count,
                                      micro_bursts_list_t *out_bursts)
{
    int ret_val;
    int point_idx;
    int event_idx;
    int target_cluster;
    int slot;
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
            for (point_idx = 0; point_idx < points->len; point_idx++)
            {
                target_cluster = points->arr[point_idx].cluster_id;
                if (target_cluster >= FIRST_CLUSTER && target_cluster <= cluster_count)
                {
                    total_size = bursts_buffer[target_cluster - FIRST_CLUSTER].member_count + 1;
                    bursts_buffer[target_cluster - FIRST_CLUSTER].member_count = total_size;
                }
            }

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
                for (point_idx = 0; point_idx < points->len; point_idx++)
                {
                    target_cluster = points->arr[point_idx].cluster_id;
                    if (target_cluster >= FIRST_CLUSTER && target_cluster <= cluster_count)
                    {
                        event_idx = points->arr[point_idx].original_index;
                        if (event_idx >= 0 && event_idx < events->count)
                        {
                            slot = write_cursor[target_cluster - FIRST_CLUSTER];
                            bursts_buffer[target_cluster - FIRST_CLUSTER].member_indexes[slot] = event_idx;
                            write_cursor[target_cluster - FIRST_CLUSTER] = slot + 1;

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
                    if (bursts_buffer[cluster_iter].member_indexes != NULL)
                    {
                        free(bursts_buffer[cluster_iter].member_indexes);
                    }
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
        }

        if (write_cursor != NULL)
        {
            free(write_cursor);
        }
    }

    return ret_val;
}

void anomaly_config_defaults(anomaly_config_t *out_cfg)
{
    if (out_cfg != NULL)
    {
        out_cfg->k_ssmd = ANOMALY_DEFAULT_K_SSMD;
        out_cfg->k_z = ANOMALY_DEFAULT_K_Z;
        out_cfg->w_time = ANOMALY_DEFAULT_W_TIME;
        out_cfg->w_ssmd = ANOMALY_DEFAULT_W_SSMD;
        out_cfg->w_z = ANOMALY_DEFAULT_W_Z;
        out_cfg->macro_min_pts = ANOMALY_DEFAULT_MACRO_MIN_PTS;
        out_cfg->ewma_alpha = ANOMALY_DEFAULT_EWMA_ALPHA;
        out_cfg->micro_z_threshold = ANOMALY_DEFAULT_MICRO_Z_THRESHOLD;
        out_cfg->micro_min_pts = ANOMALY_DEFAULT_MICRO_MIN_PTS;
        out_cfg->micro_eps_bins = ANOMALY_DEFAULT_MICRO_EPS_BINS;
    }
}

void anomaly_scored_segments_init(scored_segments_list_t *out_segments)
{
    if (out_segments != NULL)
    {
        out_segments->items = NULL;
        out_segments->count = 0;
    }
}

void anomaly_scored_segments_free(scored_segments_list_t *segments)
{
    if (segments != NULL)
    {
        if (segments->items != NULL)
        {
            free(segments->items);
            segments->items = NULL;
        }
        segments->count = 0;
    }
}

void anomaly_macro_clusters_free(macro_clusters_list_t *clusters)
{
    int i;

    if (clusters != NULL)
    {
        if (clusters->items != NULL)
        {
            for (i = 0; i < clusters->count; i++)
            {
                if (clusters->items[i].member_indexes != NULL)
                {
                    free(clusters->items[i].member_indexes);
                    clusters->items[i].member_indexes = NULL;
                }
            }
            free(clusters->items);
            clusters->items = NULL;
        }
        clusters->count = 0;
    }
}

void anomaly_result_free(anomaly_result_t *result)
{
    int i;

    if (result != NULL)
    {
        anomaly_scored_segments_free(&result->macro_segments);
        anomaly_macro_clusters_free(&result->macro_clusters);

        if (result->micro_events.items != NULL)
        {
            free(result->micro_events.items);
            result->micro_events.items = NULL;
        }
        result->micro_events.count = 0;

        if (result->micro_bursts.items != NULL)
        {
            for (i = 0; i < result->micro_bursts.count; i++)
            {
                if (result->micro_bursts.items[i].member_indexes != NULL)
                {
                    free(result->micro_bursts.items[i].member_indexes);
                    result->micro_bursts.items[i].member_indexes = NULL;
                }
            }
            free(result->micro_bursts.items);
            result->micro_bursts.items = NULL;
        }
        result->micro_bursts.count = 0;
    }
}

int anomaly_generate_macro_segments(const bin_manager_t *bins,
                                    uint32_t metric_mask,
                                    double z_sensitivity,
                                    scored_segments_list_t *out_segments)
{
    int ret_val;
    int proceed;
    int metric_iter;
    int process_ok;
    metric_index_e current_metric;

    ret_val = 0;
    proceed = 1;

    if (bins != NULL && out_segments != NULL)
    {
        metric_iter = 0;
        while (metric_iter < bins->metrics_count && proceed == 1)
        {
            current_metric = bins->metrics[metric_iter];
            if (anomaly_metric_selected(metric_mask, current_metric) == 1)
            {
                process_ok = anomaly_process_one_metric(bins->bins[current_metric],
                                                       (long)bins->total_bins,
                                                       current_metric,
                                                       z_sensitivity,
                                                       out_segments);
                if (process_ok == 0)
                {
                    proceed = 0;
                }
            }
            metric_iter = metric_iter + 1;
        }

        if (proceed == 1)
        {
            ret_val = 1;
        }
    }

    return ret_val;
}

int anomaly_cluster_macro_segments(const scored_segments_list_t *segments,
                                   uint32_t metric_mask,
                                   int bin_size_ms,
                                   double pcap_duration_seconds,
                                   const anomaly_config_t *cfg,
                                   macro_clusters_list_t *out_clusters)
{
    int ret_val;
    int dbscan_result;
    int cluster_count;
    double eps_used;
    points_arr_t macro_points;

    ret_val = 0;
    macro_points.arr = NULL;
    macro_points.len = 0;
    macro_points.dim_count = 0;

    if (segments != NULL && cfg != NULL && out_clusters != NULL)
    {
        out_clusters->items = NULL;
        out_clusters->count = 0;

        if (anomaly_build_filtered_macro_points(segments,
                                                metric_mask,
                                                bin_size_ms,
                                                pcap_duration_seconds,
                                                cfg,
                                                &macro_points) == 1)
        {
            if (macro_points.len >= cfg->macro_min_pts)
            {
                dbscan_result = dbscan_process_auto(&macro_points, cfg->macro_min_pts, &eps_used);
                if (dbscan_result >= 0)
                {
                    cluster_count = anomaly_count_clusters(&macro_points);
                    if (cluster_count > 0)
                    {
                        if (anomaly_build_macro_clusters(&macro_points,
                                                         segments,
                                                         cluster_count,
                                                         out_clusters) == 1)
                        {
                            ret_val = 1;
                        }
                    }
                    else
                    {
                        // DBSCAN found only noise; zero clusters is a valid result.
                        ret_val = 1;
                    }
                }
            }
            else
            {
                // Fewer points than min_pts; cannot cluster but not an error.
                ret_val = 1;
            }

            free(macro_points.arr);
            macro_points.arr = NULL;
        }
        else
        {
            // Empty selection is a valid no-op (zero clusters).
            ret_val = 1;
        }
    }

    return ret_val;
}

int anomaly_generate_micro_events(const bin_manager_t *bins,
                                  uint32_t metric_mask,
                                  const anomaly_config_t *cfg,
                                  micro_events_list_t *out_events)
{
    int ret_val;
    int proceed;
    int metric_iter;
    int sweep_ok;
    metric_index_e current_metric;

    ret_val = 0;
    proceed = 1;

    if (bins != NULL && cfg != NULL && out_events != NULL)
    {
        metric_iter = 0;
        while (metric_iter < bins->metrics_count && proceed == 1)
        {
            current_metric = bins->metrics[metric_iter];
            if (anomaly_metric_selected(metric_mask, current_metric) == 1)
            {
                sweep_ok = anomaly_sweep_micro_for_metric(bins->bins[current_metric],
                                                         (long)bins->total_bins,
                                                         current_metric,
                                                         cfg->ewma_alpha,
                                                         cfg->micro_z_threshold,
                                                         out_events);
                if (sweep_ok == 0)
                {
                    proceed = 0;
                }
            }
            metric_iter = metric_iter + 1;
        }

        if (proceed == 1)
        {
            ret_val = 1;
        }
    }

    return ret_val;
}

int anomaly_cluster_micro_events(const micro_events_list_t *events,
                                 uint32_t metric_mask,
                                 const anomaly_config_t *cfg,
                                 micro_bursts_list_t *out_bursts)
{
    int ret_val;
    int dbscan_result;
    int cluster_count;
    double eps_used;
    points_arr_t event_points;

    ret_val = 0;
    event_points.arr = NULL;
    event_points.len = 0;
    event_points.dim_count = 0;

    if (events != NULL && cfg != NULL && out_bursts != NULL)
    {
        out_bursts->items = NULL;
        out_bursts->count = 0;

        if (anomaly_build_filtered_micro_points(events, metric_mask, &event_points) == 1)
        {
            if (event_points.len >= cfg->micro_min_pts)
            {
                dbscan_result = dbscan_process_auto(&event_points, cfg->micro_min_pts, &eps_used);
                if (dbscan_result >= 0)
                {
                    cluster_count = anomaly_count_clusters(&event_points);
                    if (cluster_count > 0)
                    {
                        if (anomaly_build_micro_bursts(&event_points,
                                                       events,
                                                       cluster_count,
                                                       out_bursts) == 1)
                        {
                            ret_val = 1;
                        }
                    }
                    else
                    {
                        ret_val = 1;
                    }
                }
            }
            else
            {
                ret_val = 1;
            }

            free(event_points.arr);
            event_points.arr = NULL;
        }
        else
        {
            // Empty selection: success with zero bursts.
            ret_val = 1;
        }
    }

    return ret_val;
}

int anomaly_detect_run_all(const anomaly_input_t *input, anomaly_result_t *out_result)
{
    int ret_val;
    int stage_ok;
    uint32_t mask_all;

    ret_val = 0;
    mask_all = ANOMALY_METRIC_MASK_ALL;

    if (input != NULL && input->bins != NULL && out_result != NULL)
    {
        anomaly_scored_segments_init(&out_result->macro_segments);
        out_result->macro_clusters.items = NULL;
        out_result->macro_clusters.count = 0;
        out_result->micro_events.items = NULL;
        out_result->micro_events.count = 0;
        out_result->micro_bursts.items = NULL;
        out_result->micro_bursts.count = 0;

        stage_ok = anomaly_generate_macro_segments(input->bins,
                                                   mask_all,
                                                   ANOMALY_DEFAULT_Z_SENSITIVITY,
                                                   &out_result->macro_segments);

        if (stage_ok == 1)
        {
            stage_ok = anomaly_cluster_macro_segments(&out_result->macro_segments,
                                                      mask_all,
                                                      input->bins->bin_size,
                                                      input->pcap_duration_seconds,
                                                      &input->cfg,
                                                      &out_result->macro_clusters);
        }

        if (stage_ok == 1)
        {
            stage_ok = anomaly_generate_micro_events(input->bins,
                                                    mask_all,
                                                    &input->cfg,
                                                    &out_result->micro_events);
        }

        if (stage_ok == 1)
        {
            stage_ok = anomaly_cluster_micro_events(&out_result->micro_events,
                                                   mask_all,
                                                   &input->cfg,
                                                   &out_result->micro_bursts);
        }

        if (stage_ok == 1)
        {
            ret_val = 1;
        }
        else
        {
            anomaly_result_free(out_result);
        }
    }

    return ret_val;
}
