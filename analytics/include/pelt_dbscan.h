#ifndef PELT_DBSCAN_H
#define PELT_DBSCAN_H

#include "pelt.h"
#include "point.h"
#include "dbscan.h"

#define TIME_DIM_WEIGHT 0.8
#define Z_SCORE_DIM_WEIGHT 0.1
#define SSMD_DIM_WIEGHT 0.1



typedef enum
{
    DIM_TIME,
    DIM_Z_SCORE,
    DIM_SSMD
} dim_e;

/**
 * @brief Analyzes macro segments, performing DBSCAN clustering and returning the labels.
 *
 * @param list The list of segments found by PELT.
 * @param total_duration Total duration of the PCAP.
 * @param sensitivity Sensitivity parameter for normalization.
 * @param min_pts Minimum points for DBSCAN and K for Elbow.
 * @param out_labels Pointer to an int pointer that will hold the array of cluster IDs.
 * Caller is responsible for freeing this array.
 * @param out_eps Optional pointer to store the calculated epsilon (can be NULL).
 * @return int The number of clusters found, or -1 on error.
 */
int pelt_dbscan_analyze_macro(pelt_segments_list_t *list, uint64_t total_duration,
                              double sensitivity, int min_pts, int **out_labels, double *out_eps);


#endif