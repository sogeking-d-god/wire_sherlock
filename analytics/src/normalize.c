#include <math.h>
#include <stdlib.h>
#include "normalize.h"
#include "dbscan.h"

/**
 * @brief Returns x if x >= 0, otherwise -x. Local helper to avoid pulling extra deps.
 *
 * @param value Input value.
 * @return double Absolute value.
 */
static double normalize_abs(double value)
{
    double ret_val;

    ret_val = (value < 0.0) ? -value : value;
    return ret_val;
}

/**
 * @brief Returns the larger of two doubles. Used to guard denominators from zero.
 *
 * @param value Candidate denominator.
 * @param floor_value Minimum permitted denominator.
 * @return double The larger of the two.
 */
static double normalize_guard_denominator(double value, double floor_value)
{
    double ret_val;

    ret_val = (value < floor_value) ? floor_value : value;
    return ret_val;
}

void normalize_segment_to_point(const scored_segment_t *segment, int flat_index, int bin_size_ms, double pcap_duration_seconds, const anomaly_config_t *cfg, point_t *out_point)
{
    double start_seconds;
    double duration_guarded;
    double k_ssmd_guarded;
    double k_z_guarded;
    double t_norm;
    double ssmd_norm;
    double z_norm;

    if (segment != NULL && cfg != NULL && out_point != NULL)
    {
        // Time coordinate = the PELT change-point that OPENED this segment.
        start_seconds = ((double)segment->start_bin * bin_size_ms) / NORMALIZE_MS_PER_SECOND;
        duration_guarded = normalize_guard_denominator(pcap_duration_seconds, NORMALIZE_DENOM_EPSILON);
        k_ssmd_guarded = normalize_guard_denominator(cfg->k_ssmd, NORMALIZE_DENOM_EPSILON);
        k_z_guarded = normalize_guard_denominator(cfg->k_z, NORMALIZE_DENOM_EPSILON);

        t_norm = start_seconds / duration_guarded;
        ssmd_norm = tanh(segment->ssmd / k_ssmd_guarded);
        z_norm = tanh(normalize_abs(segment->z_global) / k_z_guarded);

        out_point->vals[0] = cfg->w_time * t_norm;
        out_point->vals[1] = cfg->w_ssmd * ssmd_norm;
        out_point->vals[2] = cfg->w_z * z_norm;
        out_point->original_index = flat_index;
        out_point->cluster_id = UNCLASSIFIED;
    }
}

int normalize_build_macro_points(const scored_segments_list_t *segments, int bin_size_ms, double pcap_duration_seconds, const anomaly_config_t *cfg, points_arr_t *out_points)
{
    int ret_val;
    int i;
    point_t *buffer;

    ret_val = 0;
    buffer = NULL;

    if (segments != NULL && cfg != NULL && out_points != NULL && segments->count > 0)
    {
        buffer = (point_t *)calloc((size_t)segments->count, sizeof(point_t));
        if (buffer != NULL)
        {
            for (i = 0; i < segments->count; i++)
            {
                normalize_segment_to_point(&segments->items[i], i, bin_size_ms, pcap_duration_seconds, cfg, &buffer[i]);
            }

            out_points->arr = buffer;
            out_points->len = segments->count;
            out_points->dim_count = MAX_DIM_COUNT;
            ret_val = 1;
        }
        else
        {
            fprintf(stderr, "normalize_build_macro_points: Memory allocation failed for %d points.\n", segments->count);
        }
    }

    return ret_val;
}

void normalize_points_free(points_arr_t *points)
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
