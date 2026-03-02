#ifndef STATS_H
#define STATS_H

#include "pelt.h"
#include <math.h>

/**
 * @brief Analyzes PELT segments using Adjusted Z-Score (incorporating Standard Error).
 * (Using the precalculated prefix sums for O(1) performance.)
 * @param segments        List of segments found by PELT.
 * @param ps          Pointer to precalculated prefix sums for global statistics.
 * @param total_count Total number of data points.
 * @param sensitivity Z-score threshold (e.g., 3.0 for 99.7% confidence).
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

#endif

