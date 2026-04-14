#ifndef KD_TREE_H
#define KD_TREE_H

#include "dbscan.h"
#include "point.h"
#include "intro_select.h"


/**
 * @brief Builds a flat KD-Tree by partitioning an index array in-place.
 *
 * @param indexes The array of point indexes to be partitioned.
 * @param low Starting index of the range.
 * @param high Ending index of the range.
 * @param depth Current recursion depth.
 * @param data The master points array.
 */
void kd_tree_build_flat(int *indexes, int low, int high, int depth, points_arr_t *data);


/**
 * @brief Wrapper for flat KD-Tree range search.
 *
 * @param indexes The partitioned array of indexes (the flat tree).
 * @param n Total number of points in the indexes array.
 * @param target The point to search around.
 * @param eps Search radius.
 * @param data The master points array.
 * @param neighbor_indexes Output array for neighbor indexes.
 * @param count Pointer to the neighbor count (initialized to 0).
 */
void kd_tree_find_neighbors_flat(int *indexes, int n, point_t *target, double eps,
                                points_arr_t *data, int *neighbor_indexes, int *count);

/**
 * @brief Calculates the k-distance for each point in the dataset.
 * Used for the Elbow Method to find the optimal epsilon for DBSCAN.
 *
 * @param indexes The flat KD-Tree array.
 * @param n Total number of points.
 * @param k The k-th neighbor to find (e.g., 4).
 * @param data The master points array.
 * @param output_k_distances Output array of size n to store distances.
 */
void kd_tree_get_elbow_distances(int *indexes, int n, int k, points_arr_t *data, double *output_k_distances);


#endif