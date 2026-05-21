#ifndef STATS_H
#define STATS_H

#include "pelt.h"
#include <math.h>

/**
 * @def STATS_SSMD_DENOM_EPSILON
 * @brief Lower guard for the SSMD denominator sqrt(var_i + var_{i-1}).
 *        When both neighboring segments are essentially constant their combined
 *        variance collapses to zero, in which case SSMD is defined as 0 (no shock).
 */
#define STATS_SSMD_DENOM_EPSILON 1e-9

/**
 * @brief Analyzes PELT segments using Adjusted Z-Score (incorporating Standard Error).
 * (Using the precalculated prefix sums for O(1) performance.)
 * @param segments        List of segments found by PELT.
 * @param ps          Pointer to precalculated prefix sums for global statistics.
 * @param total_count Total number of data points.
 * @param sensitivity Z-score threshold
 */
void stats_analyze_with_standard_error(pelt_segments_list_t *segments, prefix_sums_t *ps, uint64_t total_count, double sensitivity);

/**
 * @brief Applies Exponential Weighted Moving Average (EWMA) to smooth data.
 *
 * @param input Raw data array.
 * @param output Smoothed data array.
 * @param n Number of elements.
 * @param alpha Smoothing factor (0 < alpha < 1). Lower is smoother.
 */
void stats_ewma_filter(double *input, double *output, uint64_t n, double alpha);

/**
 * @brief Computes SSMD (Strictly Standardized Mean Difference) for each PELT segment
 *        against its immediately preceding segment, writing the result into seg->ssmd.
 *
 * Formula for segment i (i >= 1):
 *   SSMD_i = |mean_i - mean_{i-1}| / sqrt(var_i + var_{i-1})
 *
 * The first segment has no predecessor, so its SSMD is defined as 0.
 * When the combined variance under the square root is below STATS_SSMD_DENOM_EPSILON,
 * SSMD is also set to 0 to avoid amplifying numerical noise on flat-constant regions.
 *
 * @param segments PELT segments list whose mean/variance fields are already populated.
 */
void stats_compute_ssmd_against_prev(pelt_segments_list_t *segments);

#endif

