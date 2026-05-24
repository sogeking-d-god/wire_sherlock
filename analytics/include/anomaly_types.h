#ifndef ANOMALY_TYPES_H
#define ANOMALY_TYPES_H

#include <stdint.h>
#include "time_series.h"

/**
 * @def ANOMALY_DEFAULT_K_SSMD
 * @brief Saturation point for the SSMD axis inside the tanh squash. values above it begin to saturate smoothly toward 1.0.
 */
#define ANOMALY_DEFAULT_K_SSMD 5.0

/**
 * @def ANOMALY_DEFAULT_K_Z
 * @brief Saturation point for the global Z-score axis inside the tanh squash.
 */
#define ANOMALY_DEFAULT_K_Z 10.0

/**
 * @def ANOMALY_DEFAULT_W_TIME
 * @brief Per-axis Euclidean weight for the normalized time dimension in Macro DBSCAN.
 */
#define ANOMALY_DEFAULT_W_TIME 1.0

/**
 * @def ANOMALY_DEFAULT_W_SSMD
 * @brief Per-axis Euclidean weight for the SSMD dimension in Macro DBSCAN.
 */
#define ANOMALY_DEFAULT_W_SSMD 1.0

/**
 * @def ANOMALY_DEFAULT_W_Z
 * @brief Per-axis Euclidean weight for the global Z dimension in Macro DBSCAN.
 */
#define ANOMALY_DEFAULT_W_Z 1.0

/**
 * @def ANOMALY_DEFAULT_MACRO_MIN_PTS
 * @brief Minimum number of segments needed to form a Macro cluster.
 */
#define ANOMALY_DEFAULT_MACRO_MIN_PTS 3

/**
 * @def ANOMALY_DEFAULT_EWMA_ALPHA
 * @brief Smoothing factor for the streaming EWMA mean/variance in the Micro channel.
 */
#define ANOMALY_DEFAULT_EWMA_ALPHA 0.05

/**
 * @def ANOMALY_DEFAULT_MICRO_Z_THRESHOLD
 * @brief Absolute sliding Z-score above which a single bin is flagged as a Micro event.
 */
#define ANOMALY_DEFAULT_MICRO_Z_THRESHOLD 5.0

/**
 * @def ANOMALY_DEFAULT_MICRO_MIN_PTS
 * @brief Minimum number of Micro events needed to form a Burst cluster.
 */
#define ANOMALY_DEFAULT_MICRO_MIN_PTS 3

/**
 * @def ANOMALY_DEFAULT_MICRO_EPS_BINS
 * @brief DBSCAN epsilon (in bin units) for the 1D temporal clustering of Micro events.
 */
#define ANOMALY_DEFAULT_MICRO_EPS_BINS 5

/**
 * @brief One PELT segment after scoring with SSMD (vs predecessor) and global Z-score (vs file-wide mean). Pre-normalization.
 */
typedef struct
{
    metric_index_e metric;
    uint64_t start_bin;
    uint64_t end_bin;
    double mean;
    double variance;
    double ssmd;
    double z_global;
} scored_segment_t;

/**
 * @brief Flat container of scored segments fused from all metrics.
 */
typedef struct
{
    scored_segment_t *items;
    int count;
} scored_segments_list_t;

/**
 * @brief A Macro cluster produced by DBSCAN over the fused 3D point set.  member_indexes references entries in the flat scored_segments_list_t.
 */
typedef struct
{
    int cluster_id;
    uint64_t time_start_bin;
    uint64_t time_end_bin;
    int *member_indexes;
    int member_count;
    double max_abs_ssmd;
    double max_abs_z;
} macro_cluster_t;

/**
 * @brief Container of Macro clusters discovered for one PCAP analysis.
 */
typedef struct
{
    macro_cluster_t *items;
    int count;
} macro_clusters_list_t;

/**
 * @brief A single bin flagged by the sliding EWMA Z-score in the Micro channel.
 */
typedef struct
{
    metric_index_e metric;
    uint64_t bin_index;
    double value;
    double z_sliding;
} micro_event_t;

/**
 * @brief Container of all per-bin Micro events across metrics.
 */
typedef struct
{
    micro_event_t *items;
    int count;
} micro_events_list_t;

/**
 * @brief A temporal cluster of Micro events (a Burst). member_indexes references entries in the flat micro_events_list_t.
 */
typedef struct
{
    int cluster_id;
    uint64_t bin_start;
    uint64_t bin_end;
    int *member_indexes;
    int member_count;
} micro_burst_t;

/**
 * @brief Container of Bursts discovered in the Micro channel.
 */
typedef struct
{
    micro_burst_t *items;
    int count;
} micro_bursts_list_t;

/**
 * @brief Aggregate result of one anomaly detection pass over a PCAP. Owns all four nested lists and their backing arrays.
 */
typedef struct
{
    scored_segments_list_t macro_segments;
    macro_clusters_list_t macro_clusters;
    micro_events_list_t micro_events;
    micro_bursts_list_t micro_bursts;
} anomaly_result_t;

/**
 * @brief Tunable knobs for the full anomaly detection pipeline. Populated with safe defaults by anomaly_config_defaults().
 */
typedef struct
{
    double k_ssmd;
    double k_z;
    double w_time;
    double w_ssmd;
    double w_z;
    int macro_min_pts;
    double ewma_alpha;
    double micro_z_threshold;
    int micro_min_pts;
    int micro_eps_bins;
} anomaly_config_t;

#endif
