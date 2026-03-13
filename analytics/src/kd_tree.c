#include "kd_tree.h"

static double calculate_distance(point_t *p1, point_t *p2, int dim_count)
{
    double sum = 0, diff;
    for (int i = 0; i < dim_count; i++)
    {
        diff = p1->vals[i] - p2->vals[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}

/**
 * @brief Recursively builds a balanced KD-Tree.
 *
 * @param indexes Array of indexes to points.
 * @param n Number of indexes.
 * @param depth Current depth in the tree (used to determine split dimension).
 * @param data The original points data.
 * @return kd_node_t* Pointer to the root of the (sub)tree.
 */
kd_node_t* kdtree_build_recursive(int *indexes, int n, int depth, points_arr_t *data)
{
    kd_node_t *node = NULL;

    int dim = depth % data->dim_count;
    int median_idx = n / 2;

    if (n > 0)
    {
        intro_select(indexes, n, median_idx, data, dim);

        node = (kd_node_t*)malloc(sizeof(kd_node_t));
        if (!node)
        {
            printf("error: kd tree node malloc failed at kdtree_build_recursive!\n");
        }
        else
        {
            node->point_index = indexes[median_idx];
            node->left = kdtree_build_recursive(indexes, median_idx, depth + 1, data);
            node->right = kdtree_build_recursive(indexes + median_idx + 1, n - median_idx - 1, depth + 1, data);
        }
    }
    return node;
}

void kd_find_neighbors(kd_node_t *root, point_t *target, double eps,
                       int depth, points_arr_t *data,
                       int *neighbor_indexes, int *count)
{
    point_t node_point;
    double dist, diff;
    int axis;

    if (root != NULL)
    {
        node_point = data->arr[root->point_index];
        dist = calculate_distance(&node_point, target, data->dim_count);

        if (dist <= eps)
        {
            neighbor_indexes[(*count)++] = root->point_index;
        }

        axis = depth % data->dim_count;
        diff = target->vals[axis] - node_point.vals[axis];

        // Search left subtree
        if (diff <= eps)
        {
            kd_find_neighbors(root->left, target, eps, depth + 1, data, neighbor_indexes, count);
        }

        // Search right subtree
        if (diff >= -eps)
        {
            kd_find_neighbors(root->right, target, eps, depth + 1, data, neighbor_indexes, count);
        }
    }
}