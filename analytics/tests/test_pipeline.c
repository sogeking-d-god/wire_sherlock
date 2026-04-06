#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "kd_tree.h"
#include "dbscan.h"
#include "pelt_dbscan.h"

/* --- Helper func for mocking points --- */
void init_mock_points(points_arr_t *geom, int n) {
    geom->len = n;
    geom->dim_count = MAX_DIM_COUNT;
    geom->arr = (point_t*)malloc(sizeof(point_t) * n);
    for(int i = 0; i < n; i++) {
        geom->arr[i].original_index = i;
        geom->arr[i].cluster_id = UNCLASSIFIED;
    }
}

/* --- 1. Test KD-Tree Build and Basic Find --- */
void test_kd_tree_basics() {
    printf("Running test_kd_tree_basics...\n");
    points_arr_t geom;
    init_mock_points(&geom, 3);

    geom.arr[0].vals[0] = 1.0; geom.arr[0].vals[1] = 1.0; geom.arr[0].vals[2] = 1.0;
    geom.arr[1].vals[0] = 1.1; geom.arr[1].vals[1] = 1.1; geom.arr[1].vals[2] = 1.1;
    geom.arr[2].vals[0] = 9.0; geom.arr[2].vals[1] = 9.0; geom.arr[2].vals[2] = 9.0;

    int indexes[] = {0, 1, 2};
    kd_tree_build_flat(indexes, 0, 2, 0, &geom);

    int neighbor_buffer[3];
    int count = 0;

    kd_tree_find_neighbors_flat(indexes, 3, &geom.arr[0], 0.5, &geom, neighbor_buffer, &count);
    assert(count == 2);

    free(geom.arr);
    printf("PASS\n");
}

/* --- 2. Test Auto DBSCAN (Elbow + Clustering) --- */
void test_dbscan_auto_pipeline() {
    printf("Running test_dbscan_auto_pipeline...\n");
    points_arr_t geom;
    init_mock_points(&geom, 5);

    geom.arr[0].vals[0] = 0.0; geom.arr[0].vals[1] = 0.0; geom.arr[0].vals[2] = 0.0;
    geom.arr[1].vals[0] = 0.1; geom.arr[1].vals[1] = 0.1; geom.arr[1].vals[2] = 0.1;
    geom.arr[2].vals[0] = 0.2; geom.arr[2].vals[1] = 0.2; geom.arr[2].vals[2] = 0.2;
    geom.arr[3].vals[0] = 5.0; geom.arr[3].vals[1] = 5.0; geom.arr[3].vals[2] = 5.0;
    geom.arr[4].vals[0] = 9.0; geom.arr[4].vals[1] = 9.0; geom.arr[4].vals[2] = 9.0;

    double calc_eps = 0.0;
    int min_pts = 2;

    int clusters = dbscan_process_auto(&geom, min_pts, &calc_eps);

    assert(clusters == 1);
    assert(geom.arr[0].cluster_id == FIRST_CLUSTER);
    assert(geom.arr[3].cluster_id == NOISE);

    free(geom.arr);
    printf("PASS (Calculated Eps: %f)\n", calc_eps);
}

/* --- 3. Test Full Wrapper (Mocking PELT input) --- */
void test_pelt_wrapper_macro() {
    printf("Running test_pelt_wrapper_macro...\n");

    pelt_segments_list_t list;
    list.count = 4;
    list.segments = (pelt_segment_t*)malloc(sizeof(pelt_segment_t) * 4);

    list.segments[0].start = 0;   list.segments[0].mean = 10; list.segments[0].variance = 1.0; list.segments[0].z_score = 0.1;
    list.segments[1].start = 100; list.segments[1].mean = 11; list.segments[1].variance = 1.2; list.segments[1].z_score = 0.2;

    // Jump representing an attack
    list.segments[2].start = 200; list.segments[2].mean = 50; list.segments[2].variance = 2.0; list.segments[2].z_score = 6.0;
    list.segments[3].start = 300; list.segments[3].mean = 52; list.segments[3].variance = 2.1; list.segments[3].z_score = 6.2;

    int *labels = NULL;
    double eps = 0.0;
    int status = pelt_dbscan_analyze_macro(&list, 1000, 5.0, 2, &labels, &eps);

    assert(status >= 0);
    assert(labels != NULL);

    for(int i = 0; i < 4; i++) {
        assert(labels[i] >= NOISE);
    }

    free(labels);
    free(list.segments);
    printf("PASS (Clusters found: %d)\n", status);
}

/* --- 4. Test Zero Variance Edge Case --- */
void test_zero_variance_edge_case() {
    printf("Running test_zero_variance_edge_case...\n");

    pelt_segments_list_t list;
    list.count = 2;
    list.segments = (pelt_segment_t*)malloc(sizeof(pelt_segment_t) * 2);

    // Both segments have exactly 0 variance, but the mean jumps from 0 to 100
    list.segments[0].start = 0;   list.segments[0].mean = 0;   list.segments[0].variance = 0.0; list.segments[0].z_score = 0.0;
    list.segments[1].start = 100; list.segments[1].mean = 100; list.segments[1].variance = 0.0; list.segments[1].z_score = 10.0;

    int *labels = NULL;
    double eps = 0.0;
    double sensitivity = 5.0;

    int status = pelt_dbscan_analyze_macro(&list, 200, sensitivity, 2, &labels, &eps);

    // If the division by zero protection failed, this will crash before reaching here.
    assert(status >= 0);
    assert(labels != NULL);

    free(labels);
    free(list.segments);
    printf("PASS (Handled Zero Variance Successfully)\n");
}

int main() {
    printf("--- Starting Core AI Pipeline Tests ---\n");
    test_kd_tree_basics();
    test_dbscan_auto_pipeline();
    test_pelt_wrapper_macro();
    test_zero_variance_edge_case();
    printf("--- All Tests Completed Successfully ---\n");
    return 0;
}