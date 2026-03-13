#ifndef POINT_H
#define POINT_H

/**
 * @brief A point in time with multiple dimentions
 */
typedef struct
{
    double * vals;
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