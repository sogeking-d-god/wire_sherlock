#ifndef MICRO_DETECTOR_H
#define MICRO_DETECTOR_H

#include <stdint.h>
#include "anomaly_types.h"
#include "time_series.h"

/**
 * @def MICRO_EWMA_WARMUP_BINS
 * @brief Number of leading bins suppressed from event emission so the streaming
 *        EWMA mean and variance stabilize before being used as a baseline.
 *        Bins below this index never produce micro events even if their raw
 *        sliding Z appears extreme (because the baseline is not yet trustworthy).
 */
#define MICRO_EWMA_WARMUP_BINS 10

/**
 * @def MICRO_EWMA_VAR_FLOOR
 * @brief Lower guard for the streaming EWMA variance under the square root.
 *        Prevents division by zero when the trailing window has been perfectly flat.
 */
#define MICRO_EWMA_VAR_FLOOR 1e-9

/**
 * @def MICRO_ONE_MINUS_DEFAULT
 * @brief Constant 1.0 used as the complement of the EWMA smoothing factor.
 *        Kept as a named symbol to avoid a bare literal in the update step.
 */
#define MICRO_ONE_MINUS_DEFAULT 1.0

/**
 * @def MICRO_EVENT_DIM_COUNT
 * @brief Dimensionality of the points_arr_t used for Micro burst clustering.
 *        The Micro channel clusters events only by their temporal position, so
 *        the KD-tree operates as a true 1D structure (axis = bin_index).
 *        Using dim_count = 1 (instead of padding vals[1..2] with zeros and
 *        leaving dim_count = 3) prevents the tree from wasting depth on
 *        degenerate splits along constant-zero axes.
 */
#define MICRO_EVENT_DIM_COUNT 1

/**
 * @brief Runs the Micro channel end-to-end for one bin manager.
 *
 *        For every active metric the function:
 *          1. Sweeps the bin series once with a streaming EWMA mean and EWMA variance
 *             (Welford-style), comparing each bin against the PRIOR-step mean so the
 *             current spike never contaminates its own baseline.
 *          2. Emits a micro_event_t for any bin whose absolute sliding Z-score
 *             exceeds cfg->micro_z_threshold (after the EWMA warmup window).
 *
 *        On success both lists are populated and owned by the caller.
 *        On allocation failure the lists are left zero-initialized.
 *
 * @param bins Source bin manager (borrowed; not modified, not freed).
 * @param cfg Pipeline configuration; reads ewma_alpha, micro_z_threshold, micro_min_pts.
 * @param out_events Output flat list of micro events across all metrics.
 * @param out_bursts Output flat list of micro bursts (temporal clusters of events).
 * @return int 1 on success, 0 on invalid input or allocation failure.
 */
int micro_detector_run(const bin_manager_t *bins, const anomaly_config_t *cfg, micro_events_list_t *out_events, micro_bursts_list_t *out_bursts);

/**
 * @brief Frees the internal items buffer of a micro_events_list_t and resets count.
 *        Safe to call on a zero-initialized list.
 *
 * @param events List whose items array is freed.
 */
void micro_events_list_free(micro_events_list_t *events);

/**
 * @brief Frees the internal items buffer of a micro_bursts_list_t, including each
 *        burst's member_indexes array, and resets count.
 *        Safe to call on a zero-initialized list.
 *
 * @param bursts List whose items array and per-burst member arrays are freed.
 */
void micro_bursts_list_free(micro_bursts_list_t *bursts);

#endif
