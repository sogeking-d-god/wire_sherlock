#ifndef POINT_H
#define POINT_H

#define MAX_DIM_COUNT 3

/**
 * @brief A point in time with multiple dimentions
 */
typedef struct
{
    double vals[MAX_DIM_COUNT];
    int original_index;
    int cluster_id; // 0: Unvisited, -1: Noise, 1+: Cluster ID
}point_t;

/**
 * @brief An array of points
 */
typedef struct
{
    int len;
    int dim_count;
    point_t * arr;
}points_arr_t;

#endif