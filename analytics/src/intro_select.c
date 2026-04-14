#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "intro_select.h"

/**
 * @brief Swaps two integer values in memory using arithmetic operations.
 *
 * @param a Pointer to the first integer.
 * @param b Pointer to the second integer.
 */
static void swap_idx(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Sorts a small array of indexes using the Insertion Sort algorithm.
 *
 * @param indexes Array of indexes to be sorted.
 * @param n Number of elements in the chunk.
 * @param data Pointer to the original points data for value comparison.
 * @param dim The dimension index to use for sorting.
 */
void sort_small_chunk(int *indexes, int n, points_arr_t *data, int dim)
{
    int i, j, key_idx;
    for (i = 1; i < n; i++)
    {
        key_idx = indexes[i];
        j = i - 1;
        while (j >= 0 && data->arr[indexes[j]].vals[dim] > data->arr[key_idx].vals[dim])
        {
            indexes[j + 1] = indexes[j];
            j--;
        }
        indexes[j + 1] = key_idx;
    }
}

/**
 * @brief Partitions the array into three parts: less than, equal to, and greater than the pivot.
 *
 * @param indexes Array of indexes to partition.
 * @param n Number of elements in the current range.
 * @param data Pointer to the original points data.
 * @param dim The dimension to compare.
 * @param pivot_val The value around which to partition.
 * @param out_low Pointer to store the start index of the "equal" range.
 * @param out_high Pointer to store the end index of the "equal" range.
 */
static void dutch_national_flag_partition_algo(int *indexes, int n, points_arr_t *data, int dim, double pivot_val, int *out_low, int *out_high)
{
    int low = 0;
    int high = n - 1;
    int i = 0;

    while (i <= high)
    {
        double current_val = data->arr[indexes[i]].vals[dim];
        if (current_val < pivot_val)
        {
            swap_idx(&indexes[i], &indexes[low]);
            low++;
            i++;
        }
        else if (current_val > pivot_val)
        {
            swap_idx(&indexes[i], &indexes[high]);
            high--;
        }
        else
        {
            i++;
        }
    }
    *out_low = low;
    *out_high = high;
}

/**
 * @brief Core recursive function for the IntroSelect algorithm.
 * Switches between QuickSelect and Median of Medians based on recursion depth.
 *
 * @param indexes Array of indexes.
 * @param n Current number of elements.
 * @param k The target rank to find.
 * @param depth_limit Remaining allowed depth before switching to Median of Medians.
 * @param data Pointer to the original points data.
 * @param dim The dimension to analyze.
 *
 * @return int The index of the k-th smallest element.
 */
static int intro_select_recursive(int *indexes, int n, int k, int depth_limit, points_arr_t *data, int dim)
{
    int i;
    int group_size;
    int num_groups;
    int mom_pivot_idx;
    double pivot_val;
    int below_pivot;
    int above_pivot;
    int ret_val;

    if (n <= 5)
    {
        sort_small_chunk(indexes, n, data, dim);
        ret_val = indexes[k];
    }
    else
    {
        // Chosing a pivot
        if (depth_limit > 0)
        {
            // QuickSelect: Picks a random element as pivot
            pivot_val = data->arr[indexes[rand() % n]].vals[dim];
        }
        else
        {
            // Median of Medians: Guaranteed to select approximate median of at least 0.3 n as pivot
            num_groups = 0;
            for (i = 0; i < n; i += 5)
            {
                // check if last group and calculate size (if not last group 5)
                group_size = (i + 5 <= n) ? 5 : (n - i);
                sort_small_chunk(indexes + i, group_size, data, dim);

                // move all of the medians of the chuncks to the start of the array (Allows avoiding malloc by reusing the array)
                swap_idx(&indexes[num_groups], &indexes[i + group_size / 2]);
                num_groups++;
            }
            mom_pivot_idx = intro_select_recursive(indexes, num_groups, num_groups / 2, 0, data, dim);
            pivot_val = data->arr[mom_pivot_idx].vals[dim];
        }

        dutch_national_flag_partition_algo(indexes, n, data, dim, pivot_val, &below_pivot, &above_pivot);

        // Pruning (checking in wich third k is located)
        if (k < below_pivot)
        {
            ret_val = intro_select_recursive(indexes, below_pivot, k, depth_limit - 1, data, dim);
        }
        else if (k > above_pivot)
        {
            ret_val = intro_select_recursive(indexes + above_pivot + 1, n - above_pivot - 1, k - above_pivot - 1, depth_limit - 1, data, dim);
        }
        else
        {
            ret_val = indexes[k];
        }
    }

    return ret_val;
}

int intro_select(int *indexes, int n, int k, points_arr_t *data, int dim)
{
    int depth_limit;

    if (n <= 0 || k < 0 || k >= n) return -1;

    depth_limit = 2 * (int)log2(n);

    return intro_select_recursive(indexes, n, k, depth_limit, data, dim);
}