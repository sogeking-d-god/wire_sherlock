#ifndef PELT_H
#define PELT_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <float.h>

/**
 * @brief Container for the raw data to be analyzed.
 */
typedef struct
{
    double *data;
    uint64_t count;
} time_series_t;

/**
 * @brief struct that holds precalculated the sums of all elements to make cost calculation O(1)
 *
 */
typedef struct
{
    double *sum; // an array of the sums, sum[i] = sum of frist i elements => from element[0] to [i -1]
    double *sum_sq; // an array of sums squered, sum_sq[i] => from element[0]^2 to [i -1]^2
} prefix_sums_t;

/**
 * @brief Represents a single detected segment between two changepoints.
 *        z_score is the segment-vs-global Z statistic (filled by stats_analyze_with_standard_error).
 *        ssmd is the Strictly Standardized Mean Difference vs the immediately preceding segment
 *        (filled by stats_compute_ssmd_against_prev). The first segment has ssmd = 0.
 */
typedef struct
{
    uint64_t start;
    uint64_t end;
    double mean;
    double variance;
    double z_score;
    double ssmd;
    int is_anomaly;
} pelt_segment_t;

/**
 * @brief List of all segments found by the algorithm.
 */
typedef struct
{
    pelt_segment_t *segments;
    int count;
} pelt_segments_list_t;

/**
 * @brief Detects changepoints in a time series using the PELT algorithm.
 *
 * @param elements Pointer to the time series data.
 * @param penalty The penalty value (usually calculated via BIC).
 * @return pelt_segments_list_t Struct containing the detected segments.
 */
pelt_segments_list_t pelt_get_segments(time_series_t *ts, int *changepoints);

/**
 * @brief Calculates the BIC (Bayesian Information Criterion) penalty.
 **STATISTICAL CONTEXT:
 *      BIC is a "strict" model that avoids over-fitting.
 *      It is ideal for large datasets because the penalty
 *      grows with log(n).
 *
 * @param n Number of data points.
 * @param p Number of parameters (usually 1 for mean change).
 * @return double The calculated penalty value.
 */
double pelt_calculate_penalty_BIC(uint64_t n, int p);

/**
 * @brief Detects changepoints in a time series using the PELT algorithm.
 *
 * @param elements Pointer to the time series data.
 * @param penalty The penalty value (usually calculated via BIC).
 * @return pelt_segments_list_t Struct containing the detected segments, and its stsistics.
 */
pelt_segments_list_t pelt_detect_changepoints(time_series_t *elements, double penalty);


#endif