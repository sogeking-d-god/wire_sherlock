#ifndef NORMALIZE_H
#define NORMALIZE_H

#include "anomaly_types.h"
#include "point.h"

/**
 * @def NORMALIZE_DENOM_EPSILON
 * @brief Lower guard for any denominator (pcap duration, saturation knees).
 *        Prevents division by zero on degenerate inputs.
 */
#define NORMALIZE_DENOM_EPSILON 1e-9

/**
 * @def NORMALIZE_MS_PER_SECOND
 * @brief Millisecond-to-second conversion factor used when expressing bin width in seconds.
 */
#define NORMALIZE_MS_PER_SECOND 1000.0

/**
 * @def NORMALIZE_BIN_MIDPOINT_OFFSET
 * @brief Additive offset applied when converting an inclusive [start, end] bin
 *        range into a midpoint in bin units: mid = (start + end + 1) / 2.
 */
#define NORMALIZE_BIN_MIDPOINT_OFFSET 1.0

/**
 * @def NORMALIZE_HALF
 * @brief Divisor used to take the midpoint of two bin indexes.
 */
#define NORMALIZE_HALF 2.0

/**
 * @brief Computes the midpoint time (in seconds, from the start of the PCAP) of a segment
 *        spanning bins [start_bin, end_bin] given the bin width in milliseconds.
 *
 * @param start_bin Inclusive index of the first bin in the segment.
 * @param end_bin Inclusive index of the last bin in the segment.
 * @param bin_size_ms Width of a single bin in milliseconds.
 * @return double Midpoint time in seconds.
 */
double normalize_segment_midpoint_seconds(uint64_t start_bin, uint64_t end_bin, int bin_size_ms);

/**
 * @brief Maps one scored segment into a 3D point in normalized Euclidean space.
 *
 * The transform is:
 *   t_norm    = midpoint_seconds / pcap_duration_seconds
 *   ssmd_norm = tanh(ssmd / cfg->k_ssmd)
 *   z_norm    = tanh(|z_global| / cfg->k_z)
 * with per-axis weights applied to keep DBSCAN epsilon interpretable.
 *
 * @param segment Segment to project.
 * @param flat_index Index of the segment in the flat scored_segments_list_t (stored in original_index).
 * @param bin_size_ms Width of a single bin in milliseconds.
 * @param pcap_duration_seconds Total duration of the PCAP file in seconds.
 * @param cfg Pipeline configuration; supplies saturation knees and axis weights.
 * @param out_point Output point; vals[0..2] populated, original_index set, cluster_id zeroed.
 */
void normalize_segment_to_point(const scored_segment_t *segment,
                                int flat_index,
                                int bin_size_ms,
                                double pcap_duration_seconds,
                                const anomaly_config_t *cfg,
                                point_t *out_point);

/**
 * @brief Allocates and fills a points_arr_t with one normalized point per scored segment.
 *        The returned array's dim_count is fixed at 3 for the Macro channel.
 *
 * @param segments Flat list of scored segments to project.
 * @param bin_size_ms Width of a single bin in milliseconds.
 * @param pcap_duration_seconds Total PCAP duration in seconds.
 * @param cfg Pipeline configuration.
 * @param out_points Output points_arr_t; arr is malloc'd, caller frees via normalize_points_free.
 * @return int 1 on success, 0 on allocation failure or invalid input.
 */
int normalize_build_macro_points(const scored_segments_list_t *segments,
                                 int bin_size_ms,
                                 double pcap_duration_seconds,
                                 const anomaly_config_t *cfg,
                                 points_arr_t *out_points);

/**
 * @brief Frees the internal arr buffer of a points_arr_t produced by normalize_build_macro_points.
 *        Safe to call on a zero-initialized container.
 *
 * @param points Points container whose arr will be freed and pointer cleared.
 */
void normalize_points_free(points_arr_t *points);

#endif
