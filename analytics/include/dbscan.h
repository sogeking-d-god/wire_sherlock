#ifndef DBSCAN_H
#define DBSCAN_H

#include "pelt.h"
#include "point.h"


#define UNCLASSIFIED  0
#define NOISE        -1
#define FIRST_CLUSTER 1

/**
 * @brief Executes the full DBSCAN pipeline: Allocates buffers, builds KD-Tree once,
 * calculates the optimal Epsilon using the Elbow method, and runs the clustering.
 *
 * @param geometry The prepared geometric points.
 * @param min_pts The minimum points for a cluster (and 'k' for the Elbow method).
 * @param out_eps Pointer to output the calculated epsilon used.
 * @return int The total number of clusters found, or -1 on error.
 */
int dbscan_process_auto(points_arr_t *geometry, int min_pts, double *out_eps);


#endif