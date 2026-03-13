#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "intro_select.h"
#include "kd_tree.h"

/* Helper to free the tree memory */
void free_kdtree(kd_node_t *node) {
    if (node == NULL) return;
    free_kdtree(node->left);
    free_kdtree(node->right);
    free(node);
}

void run_kdtree_test() {
    int i;
    int count = 0;
    double eps = 2.5;

    /* FIX: target must be a point_t to match the function signature */
    double target_coords[2] = {5.0, 5.0};
    point_t target_point;
    target_point.vals = target_coords;

    int neighbor_indexes[7];
    int indexes[7] = {0, 1, 2, 3, 4, 5, 6};

    /* Mock data: 7 points in 2D space */
    points_arr_t data;
    data.len = 7;
    data.dim_count = 2;
    data.arr = malloc(sizeof(point_t) * 7);

    double raw_vals[7][2] = {
        {5.0, 5.0}, {1.0, 1.0}, {9.0, 9.0},
        {5.5, 5.5}, {4.5, 4.5}, {2.0, 8.0}, {8.0, 2.0}
    };

    for(i = 0; i < 7; i++) {
        data.arr[i].vals = raw_vals[i];
    }

    printf("--- Starting Integration Test ---\n");
    printf("1. Testing KD-Tree Build (using IntroSelect)...\n");

    kd_node_t *root = kdtree_build_recursive(indexes, 7, 0, &data);
    assert(root != NULL);
    printf("Result: Tree built successfully.\n");

    printf("\n2. Testing Neighbor Search (Range Search)...\n");
    printf("Searching for points within EPS=%.1f of (%.1f, %.1f)...\n", eps, target_coords[0], target_coords[1]);

    /* Updated call with pointer to target_point */
    kd_find_neighbors(root, &target_point, eps, 0, &data, neighbor_indexes, &count);

    printf("Found %d neighbors:\n", count);
    for(i = 0; i < count; i++) {
        int idx = neighbor_indexes[i];
        printf("   - Point %d: (%.1f, %.1f)\n", idx, data.arr[idx].vals[0], data.arr[idx].vals[1]);
    }

    /* Logic check: In this dataset, (5,5), (5.5, 5.5), and (4.5, 4.5) are within EPS 2.5 */
    assert(count >= 3);
    printf("Result: Neighborhood logic is correct.\n");

    /* Cleanup */
    free_kdtree(root);
    free(data.arr);
    printf("\n--- All Tests Passed Successfully! ---\n");
}

int main() {
    /* Initialize random seed for intro_select's QuickSelect phase */
    srand(42);
    run_kdtree_test();
    return 0;
}