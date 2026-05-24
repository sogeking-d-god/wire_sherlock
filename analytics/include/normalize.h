#ifndef NORMALIZE_H
#define NORMALIZE_H

#include "anomaly_types.h"
#include "point.h"

/**
 * @def NORMALIZE_DENOM_EPSILON
 * @brief Lower guard for any denominator (pcap duration, saturation knees).
 *        Prevents division by zero.
 */
#define NORMALIZE_DENOM_EPSILON 1e-9

/**
 * @def NORMALIZE_MS_PER_SECOND
 * @brief Millisecond-to-second conversion factor used when expressing bin width in seconds.
 */
#define NORMALIZE_MS_PER_SECOND 1000.0

/**
 * @brief Maps one scored segment into a 3D point in normalized Euclidean space.
 *
 *        The time axis uses the segment's START bin (the PELT change-point that
 *        OPENED the segment). The Macro channel exists to find
 *        co-occurring shifts ACROSS metrics, and "co-occurring" means change-points
 *        firing at the same instant. A long sustained segment and a brief spike
 *        triggered at the same moment must share the same time coordinate to be
 *        cluster-able; using the midpoint would push them apart by half the long
 *        segment's length.
 *
 * The transform is:
 *   t_norm    = start_seconds / pcap_duration_seconds
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
void normalize_segment_to_point(const scored_segment_t *segment, int flat_index, int bin_size_ms, double pcap_duration_seconds, const anomaly_config_t *cfg, point_t *out_point);

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
