#include "stats.h"

void stats_analyze_with_standard_error(pelt_segments_list_t *segments, prefix_sums_t *ps, uint64_t total_count, double sensitivity)
{
    double global_sum = ps->sum[total_count];
    double global_sum_sq = ps->sum_sq[total_count];
    double global_mean = global_sum / total_count;
    double global_var = (global_sum_sq / total_count) - (global_mean * global_mean);
    double global_std_dev = sqrt(global_var);

    pelt_segment_t *seg;
    uint64_t n_seg;
    double standard_error;
    double z_adj;

    if (global_std_dev >= 0.00001)
    {
        for (int i = 0; i < segments->count; i++)
        {
            seg = &segments->segments[i];
            n_seg = seg->end - seg->start + 1;

            // Z_adj = (MeanDiff) / (StdDev / sqrt(N))
            standard_error = global_std_dev / sqrt((double)n_seg);
            z_adj = (seg->mean - global_mean) / standard_error;

            seg->z_score = z_adj;
            seg->is_anomaly = (((z_adj > 0) ? z_adj : - z_adj) > sensitivity) ? 1 : 0;
        }
    }
}

void stats_ewma_filter(double *input, double *output, uint64_t n, double alpha)
{
    if (n != 0)
    {
        output[0] = input[0]; // Start with the first value
        for (uint64_t i = 1; i < n; i++)
        {
            output[i] = (alpha * input[i]) + ((1.0 - alpha) * output[i - 1]);
        }
    }
}

void stats_compute_ssmd_against_prev(pelt_segments_list_t *segments)
{
    int i;
    pelt_segment_t *current_segment;
    pelt_segment_t *previous_segment;
    double mean_difference;
    double combined_variance;
    double denominator;

    if (segments != NULL && segments->segments != NULL && segments->count > 0)
    {
        // First segment has no predecessor
        segments->segments[0].ssmd = 0.0;

        for (i = 1; i < segments->count; i++)
        {
            current_segment = &segments->segments[i];
            previous_segment = &segments->segments[i - 1];

            mean_difference = current_segment->mean - previous_segment->mean;
            if (mean_difference < 0.0)
            {
                mean_difference = -mean_difference;
            }

            combined_variance = current_segment->variance + previous_segment->variance;
            denominator = sqrt(combined_variance);

            if (denominator < STATS_SSMD_DENOM_EPSILON)
            {
                current_segment->ssmd = 0.0;
            }
            else
            {
                current_segment->ssmd = mean_difference / denominator;
            }
        }
    }
}

