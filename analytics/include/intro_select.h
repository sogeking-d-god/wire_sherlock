#ifndef INTRO_SELECT_H
#define INTRO_SELECT_H

#include "point.h"

/**
 * @brief Finds the k-th smallest element in a specific dimension, and reorders the indexes array.
 * By using QuickSelect and switching to Median Of Medians to stop bad case of O(n^2).
 *
 * @param indexes Array of indexes referencing the original data points.
 * @param n Number of elements currently being processed in the indexes array.
 * @param k The target index.
 * @param data Pointer to the original points array container.
 * @param dim The specific dimension index in the vals array of each point to sort and compare by.
 * @return int The index of the k-th smallest element in the original data.
 * Returns -1 if input parameters are invalid.
 */
int intro_select(int *indexes, int n, int k, points_arr_t *data, int dim);

#endif