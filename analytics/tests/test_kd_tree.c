#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "kd_tree.h" // זה הקו שחייב להיות כאן!

void test_kd_tree()
{
    int i;
    int n = 5;
    int k = 2;
    int indexes[5] = {0, 1, 2, 3, 4};
    double elbow_dists[5];
    int neighbors[5];
    int neighbor_count = 0;

    points_arr_t data;
    data.dim_count = 2;
    data.arr = (point_t*)malloc(sizeof(point_t) * n);

    printf("--- Testing Flat KD-Tree Build ---\n");
    for (i = 0; i < n; i++)
    {
        /* הנחה: vals הוא מערך סטטי בתוך point_t */
        data.arr[i].vals[0] = (double)i;
        data.arr[i].vals[1] = 0.0;
    }

    /* בניית העץ */
    kd_tree_build_flat(indexes, 0, n - 1, 0, &data);

    printf("--- Testing Range Search (eps=1.1) ---\n");
    kd_tree_find_neighbors_flat(indexes, n, &data.arr[2], 1.1, &data, neighbors, &neighbor_count);

    printf("Found %d neighbors around point 2.0\n", neighbor_count);
    assert(neighbor_count == 3);

    printf("--- Testing Elbow Distances (k=2) ---\n");
    kd_tree_get_elbow_distances(indexes, n, k, &data, elbow_dists);

    for (i = 0; i < n; i++)
    {
        printf("Point %d: %d-th distance = %.2f\n", i, k, elbow_dists[i]);
    }
    assert(elbow_dists[2] == 1.0);
    free(data.arr);
    printf("--- All Tests Passed! ---\n");
}

int main()
{
    test_kd_tree();
    return 0;
}