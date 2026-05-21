#ifndef ANOMALY_DETECTOR_H
#define ANOMALY_DETECTOR_H

#include <stdint.h>
#include "anomaly_types.h"
#include "time_series.h"
#include "pelt.h"

/**
 * @def ANOMALY_METRIC_MASK_NONE
 * @brief Empty metric mask. Selects no metrics; granular calls become no-ops.
 */
#define ANOMALY_METRIC_MASK_NONE 0U

/**
 * @def ANOMALY_METRIC_MASK_ALL
 * @brief Convenience mask selecting every metric defined by metric_index_e
 *        (bit i corresponds to metric_index_e value i). The top-level orchestrator
 *        uses this when the caller did not supply explicit per-stage masks.
 */
#define ANOMALY_METRIC_MASK_ALL ((1U << METRICS_COUNT) - 1U)

/**
 * @def ANOMALY_METRIC_BIT
 * @brief Builds a single-metric mask from a metric_index_e value.
 *        Example: ANOMALY_METRIC_BIT(METRIC_SYN_FLAG_COUNT).
 */
#define ANOMALY_METRIC_BIT(metric) (1U << ((unsigned int)(metric)))

/**
 * @def ANOMALY_PELT_PARAM_COUNT
 * @brief Number of free parameters PELT estimates per segment (mean only),
 *        passed to the BIC penalty helper.
 */
#define ANOMALY_PELT_PARAM_COUNT 1

/**
 * @def ANOMALY_DEFAULT_Z_SENSITIVITY
 * @brief Threshold passed to stats_analyze_with_standard_error when flagging
 *        segments as anomalies on the global Z axis. Kept distinct from the
 *        Micro per-bin threshold so the two channels can be tuned independently.
 */
#define ANOMALY_DEFAULT_Z_SENSITIVITY 3.0

/**
 * @brief Inputs to the top-level orchestrator. The orchestrator borrows the
 *        bin_manager (it is not modified, not freed) and reads its bins[] arrays.
 */
typedef struct
{
    const bin_manager_t *bins;
    double pcap_duration_seconds;
    anomaly_config_t cfg;
} anomaly_input_t;

/**
 * @brief Populates an anomaly_config_t with the project defaults from anomaly_types.h.
 *        Callers normally invoke this first and then override individual fields.
 *
 * @param out_cfg Configuration struct to populate; must be non-NULL.
 */
void anomaly_config_defaults(anomaly_config_t *out_cfg);

/**
 * @brief Frees every owned buffer inside an anomaly_result_t and zeros the lists.
 *        Safe to call on a partially-populated or zero-initialized result.
 *
 * @param result Result to release.
 */
void anomaly_result_free(anomaly_result_t *result);

/**
 * @brief Initializes a scored_segments_list_t to an empty state without allocating.
 *        Required before passing it to anomaly_generate_macro_segments (which appends).
 *
 * @param out_segments List to clear (items=NULL, count=0).
 */
void anomaly_scored_segments_init(scored_segments_list_t *out_segments);

/**
 * @brief Frees the items buffer of a scored_segments_list_t and resets count.
 *
 * @param segments List whose backing buffer is released.
 */
void anomaly_scored_segments_free(scored_segments_list_t *segments);

/**
 * @brief Frees a macro_clusters_list_t including each cluster's member_indexes array.
 *
 * @param clusters List to release.
 */
void anomaly_macro_clusters_free(macro_clusters_list_t *clusters);

/**
 * @brief GRANULAR STAGE 1 (Macro segment generation).
 *
 *        For every metric whose bit is set in metric_mask, this function:
 *          1. Builds a time_series_t view over bins->bins[metric] (no copy).
 *          2. Computes the BIC penalty from the bin count and ANOMALY_PELT_PARAM_COUNT.
 *          3. Runs pelt_detect_changepoints to obtain the PELT segments.
 *          4. Re-derives a prefix_sums_t (PELT freed its own) and calls
 *             stats_analyze_with_standard_error to populate the Z_global score.
 *          5. Calls stats_compute_ssmd_against_prev to populate SSMD vs the prior segment.
 *          6. Appends every produced segment, tagged with its metric, into out_segments.
 *
 *        out_segments MUST have been initialized with anomaly_scored_segments_init
 *        (or be the result of an earlier call to this function). The list is grown
 *        incrementally so multiple calls with different masks compose cleanly.
 *
 * @param bins Source bin_manager containing the per-metric time series.
 * @param metric_mask Bitmask of metric_index_e values to process. Bits outside
 *                    [0, METRICS_COUNT) are ignored.
 * @param z_sensitivity Threshold passed to stats_analyze_with_standard_error.
 * @param out_segments Append-only output list of scored segments across all selected metrics.
 * @return int 1 on success, 0 on invalid input or allocation failure.
 */
int anomaly_generate_macro_segments(const bin_manager_t *bins,
                                    uint32_t metric_mask,
                                    double z_sensitivity,
                                    scored_segments_list_t *out_segments);

/**
 * @brief GRANULAR STAGE 2 (Macro DBSCAN clustering).
 *
 *        Projects the subset of segments whose metric belongs to metric_mask
 *        into the shared 3D normalized space (time, SSMD, Z), builds the fused
 *        KD-Tree, and runs DBSCAN. Segments outside the mask are silently skipped
 *        so the caller can cluster e.g. only PPS+SYN+RST without touching others.
 *
 *        member_indexes in the produced clusters reference positions in the
 *        FULL segments list (not the filtered subset) so downstream consumers
 *        can still look up the original segment metadata.
 *
 * @param segments Full flat list of scored segments (typically produced by stage 1).
 * @param metric_mask Bitmask selecting which segments participate in clustering.
 * @param bin_size_ms Bin width in milliseconds (taken from the source bin_manager).
 * @param pcap_duration_seconds Total PCAP duration for time-axis normalization.
 * @param cfg Pipeline configuration; reads k_ssmd, k_z, weights, macro_min_pts.
 * @param out_clusters Output cluster list; items malloc'd on success.
 * @return int 1 on success (including zero clusters), 0 on invalid input or allocation failure.
 */
int anomaly_cluster_macro_segments(const scored_segments_list_t *segments,
                                   uint32_t metric_mask,
                                   int bin_size_ms,
                                   double pcap_duration_seconds,
                                   const anomaly_config_t *cfg,
                                   macro_clusters_list_t *out_clusters);

/**
 * @brief GRANULAR STAGE 3 (Micro event detection).
 *
 *        Runs the streaming EWMA sweep ONLY on the metrics whose bit is set
 *        in metric_mask, appending the discovered micro events into out_events.
 *        out_events must be zero-initialized or previously populated by this
 *        function; the call composes additively across multiple invocations.
 *
 * @param bins Source bin_manager.
 * @param metric_mask Bitmask selecting metrics to sweep.
 * @param cfg Pipeline configuration; reads ewma_alpha and micro_z_threshold.
 * @param out_events Append-only output list of micro events.
 * @return int 1 on success, 0 on invalid input or allocation failure.
 */
int anomaly_generate_micro_events(const bin_manager_t *bins,
                                  uint32_t metric_mask,
                                  const anomaly_config_t *cfg,
                                  micro_events_list_t *out_events);

/**
 * @brief GRANULAR STAGE 4 (Micro Burst DBSCAN clustering).
 *
 *        Projects the subset of micro events whose metric belongs to metric_mask
 *        onto a 1D temporal point set (vals[0] = bin_index, other dims zeroed) and
 *        runs the existing 3D DBSCAN over it. Resulting bursts' member_indexes
 *        reference positions in the FULL events list, not the filtered subset.
 *
 * @param events Full flat list of micro events.
 * @param metric_mask Bitmask selecting which events participate in clustering.
 * @param cfg Pipeline configuration; reads micro_min_pts.
 * @param out_bursts Output bursts list; items malloc'd on success.
 * @return int 1 on success (including zero bursts), 0 on invalid input or allocation failure.
 */
int anomaly_cluster_micro_events(const micro_events_list_t *events,
                                 uint32_t metric_mask,
                                 const anomaly_config_t *cfg,
                                 micro_bursts_list_t *out_bursts);

/**
 * @brief CONVENIENCE TOP-LEVEL ORCHESTRATOR.
 *
 *        Chains all four granular stages with ANOMALY_METRIC_MASK_ALL applied to
 *        every stage, producing a fully-populated anomaly_result_t in one call.
 *        Equivalent to invoking stages 1..4 sequentially with the all-metrics mask.
 *
 *        On any sub-stage failure the partially-built result is freed and the
 *        function returns 0 with out_result zero-initialized.
 *
 * @param input Pipeline input (bins + duration + config).
 * @param out_result Aggregate result; caller frees via anomaly_result_free.
 * @return int 1 on success, 0 on invalid input or allocation failure.
 */
int anomaly_detect_run_all(const anomaly_input_t *input, anomaly_result_t *out_result);

#endif
