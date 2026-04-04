#ifndef KD_TREE_H
#define KD_TREE_H

#include "dbscan.h"
#include "point.h"
#include "intro_select.h"

/**
 * @brief A node in the KD-Tree.
 */
typedef struct kd_node_t
{
    int point_index;
    struct kd_node_t *left;
    struct kd_node_t *right;
} kd_node_t;

/**
 * @brief Recursively builds a balanced KD-Tree from an array of point indexes.
 * This function partitions the indexes based on the median of the current dimension
 * using the IntroSelect algorithm. This ensures the tree remains balanced even
 * with skewed data or many duplicates.
 *
 * @param indexes Array of indexes pointing to the original data array.
 * @param n Number of elements in the current indexes array.
 * @param depth Current recursion depth (used to determine the splitting axis).
 * @param data Pointer to the original points_arr_t structure containing all coordinates.
 * @return kd_node_t* Pointer to the newly created root of the subtree, or NULL if n <= 0
 * or memory allocation failed.
 */
kd_node_t* kd_tree_build_recursive(int *indexes, int n, int depth, points_arr_t *data);

/**
 * @brief Finds all points within radius 'eps' of a target point.
 *
 * @param root Current KD-Tree node.
 * @param target The point we are searching around.
 * @param eps The maximum distance (epsilon).
 * @param depth Current recursion depth.
 * @param data The master points array.
 * @param neighbor_indexes Output array to store found indexes.
 * @param count Pointer to the current number of neighbors found.
 */
void kd_tree_find_neighbors(kd_node_t *root, point_t *target, double eps,
                       points_arr_t *data, int *neighbor_indexes, int *count);

/**
 * @brief Frees all nodes in the KD-Tree
 *
 * @param root the root of the kd tree
 */
void kd_tree_free(kd_node_t *root);

#endif