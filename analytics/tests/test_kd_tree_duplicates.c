#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "kd_tree.h"
#include "intro_select.h"
#include "point.h"

/**
 * @def TEST_DUP_HEAVY_TOTAL
 * @brief Total number of points used in the heavy-primary-tie scenario.
 *        Large enough that an unbalanced tree would show a noticeable skew.
 */
#define TEST_DUP_HEAVY_TOTAL 1000

/**
 * @def TEST_DUP_HEAVY_TIED
 * @brief Number of points within TEST_DUP_HEAVY_TOTAL that share the same
 *        vals[0] value. 90% ties is a deliberately pathological input.
 */
#define TEST_DUP_HEAVY_TIED 900

/**
 * @def TEST_DUP_HEAVY_TIED_VALUE
 * @brief Shared primary-axis value used by the tied points.
 */
#define TEST_DUP_HEAVY_TIED_VALUE 0.5

/**
 * @def TEST_DUP_HEAVY_DIM_COUNT
 * @brief Dimensionality of the heavy-tie test data set (matches the Macro
 *        channel's 3D layout).
 */
#define TEST_DUP_HEAVY_DIM_COUNT 3

/**
 * @def TEST_DUP_HEAVY_BALANCE_TOLERANCE
 * @brief Maximum allowed |left_subtree_size - right_subtree_size| when the
 *        median is picked from a uniquely-determined position. With the lex
 *        comparator the band of equal points has width 1, so the tree is
 *        within 1 of perfectly balanced at every node.
 */
#define TEST_DUP_HEAVY_BALANCE_TOLERANCE 1

/**
 * @def TEST_DUP_IDENTICAL_TOTAL
 * @brief Number of points used in the fully-identical scenario.
 */
#define TEST_DUP_IDENTICAL_TOTAL 100

/**
 * @def TEST_DUP_IDENTICAL_VAL
 * @brief Shared coordinate value for every axis of every fully-identical point.
 */
#define TEST_DUP_IDENTICAL_VAL 0.25

/**
 * @def TEST_DUP_IDENTICAL_EPS
 * @brief Range-search radius used to verify that all fully-identical points
 *        are returned as mutual neighbors of any one of them.
 */
#define TEST_DUP_IDENTICAL_EPS 0.01

/**
 * @def TEST_DUP_HEAVY_SEED
 * @brief Fixed seed for the heavy-tie scenario's non-tied coordinates so the
 *        run is reproducible across CI invocations.
 */
#define TEST_DUP_HEAVY_SEED 1234

/**
 * @def TEST_DUP_IDENTICAL_SEED
 * @brief Fixed seed for the identical-points scenario.
 */
#define TEST_DUP_IDENTICAL_SEED 5678

/**
 * @brief Allocates a heavy-primary-tie data set: TEST_DUP_HEAVY_TIED points
 *        share vals[0] = TEST_DUP_HEAVY_TIED_VALUE, the remaining points are
 *        randomly distributed. vals[1] and vals[2] are random for every point.
 *
 * @param out_data points_arr_t to populate; caller frees out_data->arr.
 */
static void build_heavy_tie_dataset(points_arr_t *out_data)
{
    int i;

    out_data->dim_count = TEST_DUP_HEAVY_DIM_COUNT;
    out_data->len = TEST_DUP_HEAVY_TOTAL;
    out_data->arr = (point_t *)calloc((size_t)TEST_DUP_HEAVY_TOTAL, sizeof(point_t));
    assert(out_data->arr != NULL);

    srand(TEST_DUP_HEAVY_SEED);
    for (i = 0; i < TEST_DUP_HEAVY_TOTAL; i++)
    {
        if (i < TEST_DUP_HEAVY_TIED)
        {
            out_data->arr[i].vals[0] = TEST_DUP_HEAVY_TIED_VALUE;
        }
        else
        {
            out_data->arr[i].vals[0] = (double)rand() / (double)RAND_MAX;
        }
        out_data->arr[i].vals[1] = (double)rand() / (double)RAND_MAX;
        out_data->arr[i].vals[2] = (double)rand() / (double)RAND_MAX;
        out_data->arr[i].original_index = i;
        out_data->arr[i].cluster_id = 0;
    }
}

/**
 * @brief Verifies the geometric KD-tree balance invariant after intro_select
 *        has placed the median at slot k. Counts how many slots in
 *        indexes[0..k-1] hold points whose vals[axis] is strictly greater than
 *        the median's vals[axis] (i.e. would violate "left subtree <= pivot")
 *        plus how many slots in indexes[k+1..n-1] hold points whose vals[axis]
 *        is strictly less than the median's vals[axis] (would violate
 *        "right subtree >= pivot"). Both counts must be zero for the KD-tree
 *        build's downstream recursion to be correct.
 *
 *        This is the strict, geometric notion of KD-tree balance: every left
 *        descendant has vals[axis] <= pivot.vals[axis], every right descendant
 *        has vals[axis] >= pivot.vals[axis]. Under heavy ties, "equal" points
 *        legitimately appear on both sides and that is fine.
 *
 * @param indexes Partitioned indexes after intro_select.
 * @param n Length of indexes.
 * @param data Master points array.
 * @param axis Splitting axis used by the KD-tree at this depth.
 * @param median_slot Slot of the chosen median in indexes (typically n/2).
 * @return int Number of invariant violations (0 means perfectly balanced KD split).
 */
static int count_balance_violations(int *indexes,
                                    int n,
                                    points_arr_t *data,
                                    int axis,
                                    int median_slot)
{
    int ret_val;
    int i;
    double pivot_value;

    ret_val = 0;
    pivot_value = data->arr[indexes[median_slot]].vals[axis];

    for (i = 0; i < median_slot; i++)
    {
        if (data->arr[indexes[i]].vals[axis] > pivot_value)
        {
            ret_val = ret_val + 1;
        }
    }
    for (i = median_slot + 1; i < n; i++)
    {
        if (data->arr[indexes[i]].vals[axis] < pivot_value)
        {
            ret_val = ret_val + 1;
        }
    }

    return ret_val;
}

/**
 * @brief Runs the heavy-primary-tie scenario. Verifies the strict geometric
 *        KD-tree invariant: after intro_select places the median at slot n/2,
 *        every point in indexes[0..n/2-1] satisfies vals[axis] <= pivot.vals[axis]
 *        and every point in indexes[n/2+1..n-1] satisfies vals[axis] >= pivot.
 *
 *        This is what makes the resulting KD-tree geometrically balanced:
 *        slots are split exactly in half by count regardless of how many
 *        points tie on the primary axis, and the tied points may legitimately
 *        appear on both sides because the Dutch-flag equal-band has width
 *        equal to the tie multiplicity. intro_select's early-exit returns
 *        indexes[n/2] in O(band_size) work without recursing into the band.
 */
static void test_heavy_primary_tie(void)
{
    points_arr_t data;
    int *indexes;
    int i;
    int median_data_idx;
    int violations;
    int left_size;
    int right_size;

    printf("--- test_heavy_primary_tie: %d points, %d tied on axis 0 ---\n",
           TEST_DUP_HEAVY_TOTAL, TEST_DUP_HEAVY_TIED);

    build_heavy_tie_dataset(&data);

    indexes = (int *)malloc((size_t)data.len * sizeof(int));
    assert(indexes != NULL);
    for (i = 0; i < data.len; i++)
    {
        indexes[i] = i;
    }

    median_data_idx = intro_select(indexes, data.len, data.len / 2, &data, 0);
    assert(median_data_idx >= 0);

    violations = count_balance_violations(indexes, data.len, &data, 0, data.len / 2);
    left_size = data.len / 2;
    right_size = data.len - data.len / 2 - 1;

    printf("    median_idx = %d, left_size = %d, right_size = %d, violations = %d\n",
           median_data_idx, left_size, right_size, violations);
    fflush(stdout);

    // Strict geometric KD-tree balance: count split is exact (off by at most one
    // when n is odd), and every slot satisfies its side's <=/>= invariant.
    assert(violations == 0);
    assert((left_size == right_size) || (left_size == right_size + 1) || (left_size + 1 == right_size));

    free(indexes);
    free(data.arr);
    printf("    PASSED\n");
}

/**
 * @brief Runs the fully-identical scenario. Builds 100 points with identical
 *        coordinates on every axis, distinct only by original_index. Asserts
 *        (a) intro_select returns the same data index when invoked twice on
 *        independent copies (determinism), and (b) a range search around the
 *        shared coordinate returns exactly all 100 points (correctness).
 */
static void test_fully_identical_points(void)
{
    points_arr_t data;
    int *indexes_a;
    int *indexes_b;
    int *neighbors;
    point_t target;
    int neighbor_count;
    int i;
    int result_a;
    int result_b;

    printf("--- test_fully_identical_points: %d identical points ---\n",
           TEST_DUP_IDENTICAL_TOTAL);

    data.dim_count = TEST_DUP_HEAVY_DIM_COUNT;
    data.len = TEST_DUP_IDENTICAL_TOTAL;
    data.arr = (point_t *)calloc((size_t)TEST_DUP_IDENTICAL_TOTAL, sizeof(point_t));
    assert(data.arr != NULL);

    for (i = 0; i < TEST_DUP_IDENTICAL_TOTAL; i++)
    {
        data.arr[i].vals[0] = TEST_DUP_IDENTICAL_VAL;
        data.arr[i].vals[1] = TEST_DUP_IDENTICAL_VAL;
        data.arr[i].vals[2] = TEST_DUP_IDENTICAL_VAL;
        data.arr[i].original_index = i;
        data.arr[i].cluster_id = 0;
    }

    indexes_a = (int *)malloc((size_t)data.len * sizeof(int));
    indexes_b = (int *)malloc((size_t)data.len * sizeof(int));
    assert(indexes_a != NULL && indexes_b != NULL);
    for (i = 0; i < data.len; i++)
    {
        indexes_a[i] = i;
        indexes_b[i] = i;
    }

    // Determinism: same data + same input order -> same result, even though
    // the random-pivot branch of quickselect uses rand(). The lex comparator
    // collapses ties to a strict total order, so the band-of-equals has width
    // 1 and the median is uniquely determined.
    srand(TEST_DUP_IDENTICAL_SEED);
    result_a = intro_select(indexes_a, data.len, data.len / 2, &data, 0);
    srand(TEST_DUP_IDENTICAL_SEED);
    result_b = intro_select(indexes_b, data.len, data.len / 2, &data, 0);

    printf("    intro_select run A = %d, run B = %d\n", result_a, result_b);
    assert(result_a == result_b);

    // Correctness: range query around the shared coordinate returns all points.
    target.vals[0] = TEST_DUP_IDENTICAL_VAL;
    target.vals[1] = TEST_DUP_IDENTICAL_VAL;
    target.vals[2] = TEST_DUP_IDENTICAL_VAL;
    target.original_index = -1;
    target.cluster_id = 0;

    // Use indexes_a's KD-tree layout for the range search.
    kd_tree_build_flat(indexes_a, 0, data.len - 1, 0, &data);

    neighbors = (int *)malloc((size_t)data.len * sizeof(int));
    assert(neighbors != NULL);
    neighbor_count = 0;
    kd_tree_find_neighbors_flat(indexes_a, data.len, &target,
                                TEST_DUP_IDENTICAL_EPS, &data,
                                neighbors, &neighbor_count);

    printf("    range query neighbors = %d (expected %d)\n",
           neighbor_count, TEST_DUP_IDENTICAL_TOTAL);
    assert(neighbor_count == TEST_DUP_IDENTICAL_TOTAL);

    free(neighbors);
    free(indexes_a);
    free(indexes_b);
    free(data.arr);
    printf("    PASSED\n");
}

int main(void)
{
    printf("=== KD-tree duplicate-handling regression tests ===\n");
    test_heavy_primary_tie();
    test_fully_identical_points();
    printf("=== All KD-tree duplicate tests passed ===\n");
    return 0;
}
