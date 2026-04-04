#include "kd_tree.h"

/**
 * @brief the function calculates the squered distance between 2 multi dimantional points
 *
 * @param p1 first point
 * @param p2 second point
 * @param dim_count the number of dimantions that the distance will be calculated with
 * @return double the squered distance between the 2 points
 */
static double kd_tree_calculate_distance_sq(point_t *p1, point_t *p2, int dim_count)
{
    double sum = 0, diff;
    for (int i = 0; i < dim_count; i++)
    {
        diff = p1->vals[i] - p2->vals[i];
        sum += diff * diff;
    }
    return sum;
}

/**
 * @brief Recursively builds a balanced KD-Tree.
 *
 * @param indexes Array of indexes to points.
 * @param n Number of indexes.
 * @param depth Current depth in the tree (used to determine split dimension).
 * @param data The original points data.
 * @return kd_node_t* Pointer to the root of the tree or null if malloc failes.
 */
kd_node_t* kd_tree_build_recursive(int *indexes, int n, int depth, points_arr_t *data)
{
    kd_node_t *node = NULL;

    int dim = depth % data->dim_count;
    int median_idx = n / 2;
    int right_child_n, left_child_n;

    if (n > 0)
    {
        intro_select(indexes, n, median_idx, data, dim);

        node = (kd_node_t*)malloc(sizeof(kd_node_t));
        if (!node)
        {
            printf("error: kd tree node malloc failed at kd_tree_build_recursive!\n");
        }
        else
        {
            node->point_index = indexes[median_idx];

            right_child_n = n - median_idx - 1;
            left_child_n = median_idx;

            node->right = kd_tree_build_recursive(indexes + median_idx + 1, right_child_n, depth + 1, data);
            node->left = kd_tree_build_recursive(indexes, left_child_n, depth + 1, data);

            // check if current node isnt leaf and malloc truely failed
            if((right_child_n != 0 && !node->right) || (left_child_n != 0 && !node->left))
            {
                kd_tree_free(node);
                node = NULL;
            }
        }
    }
    return node;
}

void kd_tree_find_neighbors_recursive(kd_node_t *root, point_t *target, double eps, double eps_sq,
                       int depth, points_arr_t *data,
                       int *neighbor_indexes, int *count)
{
    point_t * node_point;
    double dist, diff;
    int axis;

    if (root != NULL)
    {
        node_point = &data->arr[root->point_index];
        dist = kd_tree_calculate_distance_sq(node_point, target, data->dim_count);

        if (dist <= eps_sq)
        {
            neighbor_indexes[(*count)++] = root->point_index;
        }

        axis = depth % data->dim_count;
        diff = target->vals[axis] - node_point->vals[axis];

        // Search left subtree
        if (diff <= eps)
        {
            kd_tree_find_neighbors_recursive(root->left, target, eps, eps_sq, depth + 1, data, neighbor_indexes, count);
        }

        // Search right subtree
        if (diff >= -eps)
        {
            kd_tree_find_neighbors_recursive(root->right, target, eps, eps_sq, depth + 1, data, neighbor_indexes, count);
        }
    }
}

void kd_tree_find_neighbors(kd_node_t *root, point_t *target, double eps,
                          points_arr_t *data, int *neighbor_indexes, int *count)
{
    double eps_sq = eps * eps;
    kd_tree_find_neighbors_recursive(root, target, eps, eps_sq, 0, data, neighbor_indexes, count);
}

void kd_tree_free(kd_node_t *root)
{
    if (root)
    {
        kd_tree_free(root->left);
        kd_tree_free(root->right);
        free(root);
    }
}