#include <stdlib.h>
#include "dbscan.h"
#include "kd_tree.h"

/* =========================================================
   INTERNAL HELPERS
   ========================================================= */

/**
 * @brief A helper function to expand a cluster from a core point.
 * It uses a queue to perform a breadth-first search (BFS) through the neighbors of the core point,
 * marking them as part of the same cluster if they are reachable.
 *
 * @param core_idx The index of the core point from which to expand the cluster.
 * @param cluster_id The ID of the cluster to which the points belong.
 * @param geometry The array of points to be clustered.
 * @param kd_indexes The indexes for the KD-tree.
 * @param eps The maximum neighborhood search radius.
 * @param min_pts The minimum number of points required to form a core cluster.
 * @param queue The queue for managing the BFS traversal.
 * @param neighbor_buffer The buffer for storing neighboring points.
 */
static void expand_cluster(int core_idx, int cluster_id, points_arr_t *geometry,
                          int *kd_indexes, double eps, int min_pts,
                          int *queue, int *neighbor_buffer)
{
    int q_head = 0;
    int q_tail = 0;
    int curr_idx;
    int next_idx;
    int sub_count;

    // Initial queue: add all neighbors of the core point
    kd_tree_find_neighbors_flat(kd_indexes, geometry->len, &geometry->arr[core_idx],
                               eps, geometry, neighbor_buffer, &sub_count);

    for (int j = 0; j < sub_count; j++)
    {
        if (neighbor_buffer[j] != core_idx)
        {
            queue[q_tail++] = neighbor_buffer[j];
        }
    }

    // Empty the neighbors queue and expand the cluster
    while (q_head < q_tail)
    {
        curr_idx = queue[q_head++];

        if (geometry->arr[curr_idx].cluster_id == NOISE)
        {
            geometry->arr[curr_idx].cluster_id = cluster_id;
        }
        else if (geometry->arr[curr_idx].cluster_id == UNCLASSIFIED)
        {
            geometry->arr[curr_idx].cluster_id = cluster_id;

            // Check if this neighbor is also a core point
            sub_count = 0;
            kd_tree_find_neighbors_flat(kd_indexes, geometry->len, &geometry->arr[curr_idx], eps, geometry, neighbor_buffer, &sub_count);

            if (sub_count >= min_pts)
            {
                for (int j = 0; j < sub_count; j++)
                {
                    next_idx = neighbor_buffer[j];
                    if (geometry->arr[next_idx].cluster_id <= UNCLASSIFIED)
                    {
                        queue[q_tail++] = next_idx;
                    }
                }
            }
        }
    }
}


/**
 * @brief Performs pure generic DBSCAN clustering using a flat KD-Tree.
 * Modifies the cluster_id of each point in the geometry array in-place.
 *
 * @param geometry Array of points to cluster (contains len and arr).
 * @param kd_indexes The KD-tree.
 * @param n The number of points in the geometry.
 * @param eps The maximum neighborhood search radius.
 * @param min_pts Minimum points required to form a core cluster.
 * @param queue A pre-allocated queue for BFS expansion (size should be at least n).
 * @param neighbor_buffer A pre-allocated buffer for neighbor indexes.
 * @return int The total number of valid clusters found.
 */
static int dbscan_run_internal(points_arr_t *geometry, int *kd_indexes, int n,
                               double eps, int min_pts, int *queue, int *neighbor_buffer)
{
    int current_cluster = FIRST_CLUSTER;
    int neighbor_count;

    for (int i = 0; i < n; i++)
    {
        if (geometry->arr[i].cluster_id == UNCLASSIFIED)
        {
            neighbor_count = 0;
            kd_tree_find_neighbors_flat(kd_indexes, n, &geometry->arr[i], eps, geometry, neighbor_buffer, &neighbor_count);

            if (neighbor_count < min_pts)
            {
                geometry->arr[i].cluster_id = NOISE;
            }
            else
            {
                geometry->arr[i].cluster_id = current_cluster;
                expand_cluster(i, current_cluster, geometry, kd_indexes, eps, min_pts, queue, neighbor_buffer);
                current_cluster++;
            }
        }
    }
    return current_cluster - 1;
}

/**
 * @brief The finction compares two double values for qsort in descending order.
 *
 * @param a first value
 * @param b second value
 * @return int 1 if b > a, -1 if b < a, 0 if equal
 */
static int compare_doubles_desc(const void *a, const void *b)
{
    int result = 0;
    double val_a = *(double *)a;
    double val_b = *(double *)b;

    if (val_b > val_a)
    {
        result = 1;
    }
    else if (val_b < val_a)
    {
        result = -1;
    }

    return result;
}

/**
 * @brief Finds the "elbow" point in a sorted distance array.
 * Uses the geometric approach: find the point furthest from the line
 * connecting the first and last points of the graph.
 */
static double find_elbow_point(double *distances, int n)
{
    double epsilon = 0;
    double max_dist = -1;
    double current_dist;

    // Line coordinates: P1 = (0, dist[0]), P2 = (n-1, dist[n-1])
    double x1 = 0, y1 = distances[0];
    double x2 = n - 1, y2 = distances[n - 1];

    // Pre-calculate line equation components: Ax + By + C = 0
    double A = y1 - y2;
    double B = x2 - x1;
    double C = x1 * y2 - x2 * y1;
    double denominator = sqrt(A * A + B * B);

    if (n > 2 && denominator > 0)
    {
        for (int i = 0; i < n; i++)
        {
            // Perpendicular distance from point (i, distances[i]) to the line
            current_dist = fabs(A * i + B * distances[i] + C) / denominator;
            if (current_dist > max_dist)
            {
                max_dist = current_dist;
                epsilon = distances[i];
            }
        }
    }
    else if (n > 0)
    {
        epsilon = distances[n / 2];
    }

    return epsilon;
}

/**
 * @brief The function calculates the optimal epsilon for DBSCAN using the Elbow method.
 *
 * @param geometry The prepared geometric points.
 * @param kd_indexes The KD-tree.
 * @param n The number of points.
 * @param k The number of neighbors.
 * @param k_distances An array to store the k-distances.
 * @return double The calculated epsilon value.
 */
static double calculate_epsilon_internal(points_arr_t *geometry, int *kd_indexes, int n, int k, double *k_distances)
{
   double eps = 0.0;

    if (geometry && kd_indexes && k_distances && n > 0)
    {
        kd_tree_get_elbow_distances(kd_indexes, n, k, geometry, k_distances);

        qsort(k_distances, n, sizeof(double), compare_doubles_desc);
        eps = find_elbow_point(k_distances, n);
    }

    return eps;
}

/* =========================================================
   PUBLIC API
   ========================================================= */


int dbscan_process_auto(points_arr_t *geometry, int min_pts, double *out_eps)
{
    int status = -1;
    int n;
    int clusters;
    double eps = 0;

    int *kd_indexes = NULL;
    int *queue = NULL;
    int *neighbor_buffer = NULL;
    double *k_distances = NULL;

    if (geometry && geometry->len >= min_pts && out_eps)
    {
        n = geometry->len;

        kd_indexes = (int *)malloc(n * sizeof(int));
        queue = (int *)malloc(n * sizeof(int));
        neighbor_buffer = (int *)malloc(n * sizeof(int));
        k_distances = (double *)malloc(n * sizeof(double));

        if (kd_indexes && queue && neighbor_buffer && k_distances)
        {
            for (int i = 0; i < n; i++)
            {
                geometry->arr[i].cluster_id = UNCLASSIFIED;
                kd_indexes[i] = i;
            }

            kd_tree_build_flat(kd_indexes, 0, n - 1, 0, geometry);

            eps = calculate_epsilon_internal(geometry, kd_indexes, n, min_pts, k_distances);
            *out_eps = eps;

            clusters = dbscan_run_internal(geometry, kd_indexes, n, eps, min_pts, queue, neighbor_buffer);

            status = clusters;
        }
        else
        {
            printf("error: malloc failed in dbscan_process_auto!\n");
        }

        free(kd_indexes);
        free(queue);
        free(neighbor_buffer);
        free(k_distances);
    }

    return status;
}