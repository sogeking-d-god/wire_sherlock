#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "intro_select.h"

/**
 * @def INTRO_SELECT_SMALL_CHUNK_THRESHOLD
 * @brief Sub-array size at or below which intro_select falls back to insertion
 *        sort instead of recursing. Also the group size used by the
 *        median-of-medians pivot strategy.
 */
#define INTRO_SELECT_SMALL_CHUNK_THRESHOLD 5

/**
 * @def INTRO_SELECT_DEPTH_LIMIT_FACTOR
 * @brief Multiplier on log2(n) that bounds recursion depth before switching
 *        from random-pivot quickselect to median-of-medians. The standard
 *        IntroSelect choice of 2 keeps worst-case complexity at O(n).
 */
#define INTRO_SELECT_DEPTH_LIMIT_FACTOR 2

/**
 * @brief Swaps two integer values.
 *
 * @param a Pointer to the first integer.
 * @param b Pointer to the second integer.
 */
static void swap_idx(int *a, int *b)
{
    int temp;

    temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Geometric lexicographic comparator over two points. Starts from
 *        start_dim and cycles through every axis in dim_count order. Returns 0
 *        when the two points are geometrically identical (all coordinates equal).
 *
 *
 * @param a First point.
 * @param b Second point.
 * @param start_dim Primary comparison axis.
 * @param dim_count Total number of axes in vals[].
 * @return int Negative if a precedes b geometrically, positive if b precedes a,
 *         0 when a and b share every coordinate.
 */
static int point_compare_lex(const point_t *a, const point_t *b, int start_dim, int dim_count)
{
    int ret_val;
    int offset;
    int axis;

    ret_val = 0;
    offset = 0;

    while (offset < dim_count && ret_val == 0)
    {
        axis = (start_dim + offset) % dim_count;
        if (a->vals[axis] < b->vals[axis])
        {
            ret_val = -1;
        }
        else if (a->vals[axis] > b->vals[axis])
        {
            ret_val = 1;
        }
        offset = offset + 1;
    }

    return ret_val;
}

/**
 * @brief Sorts a small chunk of indexes using insertion sort under the
 *        lexicographic point comparator. Used both as the n <= 5 base case
 *        of intro_select and as the per-group sort inside the median-of-medians
 *        pivot strategy.
 *
 * @param indexes Array of indexes to sort.
 * @param n Number of indexes in the chunk.
 * @param data Master points array (referenced by indexes).
 * @param dim Primary comparison axis passed to the lex comparator.
 */
void sort_small_chunk(int *indexes, int n, points_arr_t *data, int dim)
{
    int i;
    int j;
    int key_idx;

    for (i = 1; i < n; i++)
    {
        key_idx = indexes[i];
        j = i - 1;
        while (j >= 0 && point_compare_lex(&data->arr[indexes[j]], &data->arr[key_idx], dim, data->dim_count) > 0)
        {
            indexes[j + 1] = indexes[j];
            j = j - 1;
        }
        indexes[j + 1] = key_idx;
    }
}

/**
 * @brief Three-way Dutch-national-flag partition of indexes around a pivot
 *        identified by its slot in the indexes array. Items lex-smaller than
 *        the pivot end up in [0, *out_low - 1], items lex-equal to the pivot
 *        in [*out_low, *out_high], items lex-greater in [*out_high + 1, n - 1].
 *
 *        The pivot is stashed at position 0 for the duration of the loop so it
 *        never moves under swaps; this avoids the bookkeeping required to track
 *        a roving pivot through the high-band swaps.
 *
 *        Under the lex comparator the "equal" band only ever contains points
 *        that are truly identical to the pivot (same coords on every axis ).
 *
 * @param indexes Array of indexes covering the current sub-range.
 * @param n Number of elements in the sub-range.
 * @param data Master points array.
 * @param dim Primary axis for the lex comparator.
 * @param pivot_idx Index, within indexes[], of the chosen pivot.
 * @param out_low Output: start of the equal band.
 * @param out_high Output: end of the equal band (inclusive).
 */
static void dutch_national_flag_partition_algo(int *indexes, int n, points_arr_t *data, int dim, int pivot_idx, int *out_low, int *out_high)
{
    int low;
    int high;
    int i;
    int cmp;
    point_t *pivot_point;

    // Park the pivot at slot 0 so it stays put during the swap dance.
    swap_idx(&indexes[0], &indexes[pivot_idx]);
    pivot_point = &data->arr[indexes[0]];

    low = 1;
    high = n - 1;
    i = 1;

    while (i <= high)
    {
        cmp = point_compare_lex(&data->arr[indexes[i]], pivot_point, dim, data->dim_count);
        if (cmp < 0)
        {
            swap_idx(&indexes[i], &indexes[low]);
            low = low + 1;
            i = i + 1;
        }
        else if (cmp > 0)
        {
            swap_idx(&indexes[i], &indexes[high]);
            high = high - 1;
        }
        else
        {
            i = i + 1;
        }
    }

    // Restore the parked pivot into the head of the equal band.
    low = low - 1;
    swap_idx(&indexes[0], &indexes[low]);

    *out_low = low;
    *out_high = high;
}

/**
 * @brief Core recursive engine for IntroSelect under the lex comparator.
 *        Picks a pivot index, three-way partitions, then recurses into the
 *        band containing rank k. Switches from random-pivot quickselect to
 *        median-of-medians once depth_limit is exhausted to guarantee O(n).
 *
 * @param indexes Sub-array of indexes.
 * @param n Sub-array length.
 * @param k Target rank within the sub-array.
 * @param depth_limit Remaining quickselect depth before falling back to MoM.
 * @param data Master points array.
 * @param dim Primary axis for the lex comparator.
 * @return int The original-data index of the k-th smallest element.
 */
static int intro_select_recursive(int *indexes, int n, int k, int depth_limit, points_arr_t *data, int dim)
{
    int i;
    int group_size;
    int num_groups;
    int mom_pivot_data_idx;
    int mom_pivot_slot;
    int pivot_idx;
    int below_pivot;
    int above_pivot;
    int ret_val;

    if (n <= INTRO_SELECT_SMALL_CHUNK_THRESHOLD)
    {
        sort_small_chunk(indexes, n, data, dim);
        ret_val = indexes[k];
    }
    else
    {
        if (depth_limit > 0)
        {
            // QuickSelect: pick a random slot as pivot.
            pivot_idx = rand() % n;
        }
        else
        {
            // Median of Medians: sort groups of 5, hoist each group's median to
            // the array head, then recursively select the median of those medians.
            num_groups = 0;
            for (i = 0; i < n; i += INTRO_SELECT_SMALL_CHUNK_THRESHOLD)
            {
                group_size = (i + INTRO_SELECT_SMALL_CHUNK_THRESHOLD <= n) ? INTRO_SELECT_SMALL_CHUNK_THRESHOLD : (n - i);
                sort_small_chunk(indexes + i, group_size, data, dim);
                swap_idx(&indexes[num_groups], &indexes[i + group_size / 2]);
                num_groups = num_groups + 1;
            }
            mom_pivot_data_idx = intro_select_recursive(indexes, num_groups, num_groups / 2, 0, data, dim);

            // Locate the chosen median in the indexes array so we can pass a slot to the partition.
            mom_pivot_slot = 0;
            for (i = 0; i < n; i++)
            {
                if (indexes[i] == mom_pivot_data_idx)
                {
                    mom_pivot_slot = i;
                }
            }
            pivot_idx = mom_pivot_slot;
        }

        dutch_national_flag_partition_algo(indexes, n, data, dim, pivot_idx, &below_pivot, &above_pivot);

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
            // Early exit: k landed inside the equal-band of geometrically identical
            // points. Any slot in [below_pivot, above_pivot] satisfies "rank k", so
            // indexes[k] is a valid answer with zero further work.
            ret_val = indexes[k];
        }
    }

    return ret_val;
}

int intro_select(int *indexes, int n, int k, points_arr_t *data, int dim)
{
    int ret_val;
    int depth_limit;

    ret_val = -1;

    if (indexes != NULL && data != NULL && n > 0 && k >= 0 && k < n
        && data->dim_count > 0 && dim >= 0 && dim < data->dim_count)
    {
        depth_limit = INTRO_SELECT_DEPTH_LIMIT_FACTOR * (int)log2((double)n);
        ret_val = intro_select_recursive(indexes, n, k, depth_limit, data, dim);
    }

    return ret_val;
}
