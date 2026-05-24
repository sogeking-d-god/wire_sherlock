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

void kd_tree_build_flat(int *indexes, int low, int high, int depth, points_arr_t *data)
{
    if (low < high)
    {

        int n = high - low + 1;
        int mid = low + n / 2;
        int dim = depth % data->dim_count;

        intro_select(indexes + low, n, n / 2, data, dim);

        // build left
        kd_tree_build_flat(indexes, low, mid - 1, depth + 1, data);
        // build right
        kd_tree_build_flat(indexes, mid + 1, high, depth + 1, data);
    }
}

/**
 * @brief Finds neighbors within a radius 'eps' using a flat (array-based) KD-Tree.
 *
 * @param indexes The partitioned array of indexes (the flat tree).
 * @param low Starting index of the range.
 * @param high Ending index of the range.
 * @param target The point to search around.
 * @param eps Search radius.
 * @param eps_sq Squared search radius (eps * eps).
 * @param depth Current recursion depth.
 * @param data The master points array.
 * @param neighbor_indexes Output array for neighbor indexes.
 * @param count Pointer to the neighbor count.
 */
static void kd_tree_find_neighbors_flat_recursive(int *indexes, int low, int high, point_t *target, double eps, double eps_sq,
                                          int depth, points_arr_t *data, int *neighbor_indexes, int *count)
{
    if (low <= high)
    {

        int n = high - low + 1;
        int mid = low + n / 2;
        int axis = depth % data->dim_count;

        int current_point_idx = indexes[mid];
        point_t *node_point = &data->arr[current_point_idx];

        double diff = target->vals[axis] - node_point->vals[axis];
        double dist_sq = kd_tree_calculate_distance_sq(node_point, target, data->dim_count);
        if (dist_sq <= eps_sq)
        {
            neighbor_indexes[(*count)++] = current_point_idx;
        }

        // could be in left subtree
        if (diff <= eps)
        {
            kd_tree_find_neighbors_flat_recursive(indexes, low, mid - 1, target, eps, eps_sq, depth + 1, data, neighbor_indexes, count);
        }

        // could be in right subtree
        if (diff >= -eps)
        {
            kd_tree_find_neighbors_flat_recursive(indexes, mid + 1, high, target, eps, eps_sq, depth + 1, data, neighbor_indexes, count);
        }
    }
}

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
                                points_arr_t *data, int *neighbor_indexes, int *count)
{
    double eps_sq = eps * eps;
    *count = 0;

    if (indexes != NULL && n > 0)
    {
        kd_tree_find_neighbors_flat_recursive(indexes, 0, n - 1, target, eps, eps_sq, 0, data, neighbor_indexes, count);
    }
}

/**
 * @brief Swaps two double values.
 *
 * @param a fist val
 * @param b second val
 */
static void swap_double(double *a, double *b)
{
    double temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief the function reorders the max-heap when the new element is inserted at the head/root.
 *
 * @param heap the max heap to reorder
 * @param i the index of the head/root of unordered heap/sub-heap
 * @param k the length of the heap
 */
static void max_heapify_down(double *heap, int i, int k)
{
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < k && heap[left] > heap[largest])
    {
        largest = left;
    }

    if (right < k && heap[right] > heap[largest])
    {
        largest = right;
    }

    if (largest != i)
    {
        swap_double(&heap[i], &heap[largest]);
        max_heapify_down(heap, largest, k);
    }
}

/**
 * @brief the function reorders the max-heap when the new element is inserted at the end/tail.
 *
 * @param heap the max heap to reorder
 * @param i the index of the end/tail of unordered heap/sub-heap
 */
static void max_heapify_up(double *heap, int i)
{
    int parent = (i - 1) / 2;

    if (i > 0 && heap[i] > heap[parent])
    {
        swap_double(&heap[i], &heap[parent]);
        max_heapify_up(heap, parent);
    }
}

/**
 * @brief the function handles the insertion of a new squered distance to the k-neighbor heap.
 * by deciding between doing heapify up or down.
 *
 * @param heap the k-neighbor heap
 * @param heap_len the number of neighbors/distances already in the heap
 * @param k the length of the k-neighbor heap
 * @param new_dist_sq the new squered distance to be added to the heap
 */
static void add_distance_to_heap(double *heap, int *heap_len, int k, double new_dist_sq)
{
    if (*heap_len < k)
    {
        /* Heap not full: add to end and bubble up */
        heap[*heap_len] = new_dist_sq;
        max_heapify_up(heap, *heap_len);
        (*heap_len)++;
    }
    else if (new_dist_sq < heap[0])
    {
        /* Heap full and new dist is better: replace root and bubble down */
        heap[0] = new_dist_sq;
        max_heapify_down(heap, 0, k);
    }
}

/**
 * @brief the function searches for the k nearest neighbors of a target point using a flat KD-Tree
 *  and a max-heap to track the k closest distances found.
 *
 * @param indexes the flat KD-Tree array of point indexes
 * @param low the lower bound of the search range
 * @param high the upper bound of the search range
 * @param target the target point for which to find neighbors
 * @param depth the current depth in the tree
 * @param data the array of points
 * @param k the number of nearest neighbors to find
 * @param heap the max-heap to track the k closest distances
 * @param heap_len the current number of elements in the heap
 */
static void kd_tree_knn_flat_recursive(int *indexes, int low, int high, point_t *target, int depth, points_arr_t *data, int k, double *heap, int *heap_len)
{
    int mid, axis, near_low, near_high, far_low, far_high;
    double dist_sq, diff;
    point_t *node_point;

    if (low <= high)
    {
        mid = low + (high - low) / 2;
        axis = depth % data->dim_count;
        node_point = &data->arr[indexes[mid]];

        dist_sq = kd_tree_calculate_distance_sq(node_point, target, data->dim_count);
        add_distance_to_heap(heap, heap_len, k, dist_sq);

        // start with the child that is closer to the target point
        diff = target->vals[axis] - node_point->vals[axis];
        if (diff <= 0)
        {
            near_low = low; near_high = mid - 1;
            far_low = mid + 1; far_high = high;
        }
        else
        {
            near_low = mid + 1; near_high = high;
            far_low = low; far_high = mid - 1;
        }

        kd_tree_knn_flat_recursive(indexes, near_low, near_high, target, depth + 1, data, k, heap, heap_len);

        // search far side
        if (*heap_len < k || (diff * diff) < heap[0])
        {
            kd_tree_knn_flat_recursive(indexes, far_low, far_high, target, depth + 1, data, k, heap, heap_len);
        }
    }
}

void kd_tree_get_elbow_distances(int *indexes, int n, int k, points_arr_t *data, double *output_k_distances)
{
    int i, heap_len;
    double *heap = NULL;

    heap = (double *)malloc(sizeof(double) * k);
    if (heap != NULL)
    {
        for (i = 0; i < n; i++)
        {
            heap_len = 0;
            kd_tree_knn_flat_recursive(indexes, 0, n - 1, &data->arr[i], 0, data, k, heap, &heap_len);

            if (heap_len == k)
            {
                output_k_distances[i] = sqrt(heap[0]);
            }
            else
            {
                output_k_distances[i] = 0;
            }
        }
        free(heap);
    }
    else
    {
        printf("error: kd tree elbow heap malloc failed at kd_tree_get_elbow_distances!\n");
    }
}