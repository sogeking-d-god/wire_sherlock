#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "point.h"
#include "kd_tree.h"
// הנחה: פונקציית intro_select מוגדרת כראוי ב- intro_select.h

void run_kdtree_integration_test() {
    int i;
    int count;
    double eps;
    int neighbor_indexes[10]; // מערך לשמירת התוצאות
    int indexes[7] = {0, 1, 2, 3, 4, 5, 6};

    // 1. הגדרת נתוני דמה (Mock Data) מותאמים למבנה הסטטי
    points_arr_t data;
    data.len = 7;
    data.dim_count = 2; // עובדים ב-2 מימדים לצורך הטסט
    data.arr = (point_t*)malloc(sizeof(point_t) * 7);

    // נתוני טסט: 7 נקודות
    double raw_vals[7][2] = {
        {5.0, 5.0},  // 0: נקודת המרכז שלנו
        {1.0, 1.0},  // 1: רחוקה מאוד
        {9.0, 9.0},  // 2: רחוקה מאוד
        {5.5, 5.5},  // 3: קרובה למרכז (מרחק ריבועי: 0.5)
        {4.5, 4.5},  // 4: קרובה למרכז (מרחק ריבועי: 0.5)
        {2.0, 8.0},  // 5: רחוקה
        {5.0, 7.0}   // 6: על הגבול (מרחק מדויק 2.0 במיימד Y)
    };

    // העתקת הנתונים לתוך המערך הסטטי של ה-Struct
    for(i = 0; i < 7; i++) {
        data.arr[i].vals[0] = raw_vals[i][0];
        data.arr[i].vals[1] = raw_vals[i][1];
        data.arr[i].vals[2] = 0.0; // לא בשימוש בטסט זה
        data.arr[i].original_index = i;
        data.arr[i].cluster_id = 0;
    }

    printf("=== Starting KD-Tree Integration Tests ===\n\n");

    // --- טסט 1: בניית העץ ---
    printf("Test 1: Building KD-Tree...\n");
    kd_node_t *root = kd_tree_build_recursive(indexes, 7, 0, &data);
    assert(root != NULL); // מוודא שהעץ נבנה ולא חזר NULL
    printf("-> SUCCESS: Tree built successfully.\n\n");

    // --- טסט 2: חיפוש ברדיוס סטנדרטי (eps = 2.5) ---
    printf("Test 2: Range Search (EPS = 2.5) around (5.0, 5.0)...\n");
    point_t target_point;
    target_point.vals[0] = 5.0;
    target_point.vals[1] = 5.0;

    eps = 2.5;
    count = 0;
    kd_tree_find_neighbors(root, &target_point, eps, &data, neighbor_indexes, &count);

    printf("Found %d neighbors. Expected: 4 (indexes: 0, 3, 4, 6).\n", count);
    for(i = 0; i < count; i++) {
        int idx = neighbor_indexes[i];
        printf("   - Point %d: (%.1f, %.1f)\n", idx, data.arr[idx].vals[0], data.arr[idx].vals[1]);
    }
    // אנחנו מצפים למצוא את 0(עצמה), 3, 4, ו-6
    assert(count == 4);
    printf("-> SUCCESS: Standard range search works perfectly.\n\n");

    // --- טסט 3: חיפוש ברדיוס אפס (eps = 0.0) ---
    // מקרה קצה: אמור למצוא רק את הנקודה עצמה
    printf("Test 3: Zero Radius Search (EPS = 0.0) around (5.0, 5.0)...\n");
    eps = 0.0;
    count = 0;
    kd_tree_find_neighbors(root, &target_point, eps, &data, neighbor_indexes, &count);
    printf("Found %d neighbors. Expected: 1.\n", count);
    assert(count == 1);
    assert(neighbor_indexes[0] == 0); // חייב להיות אינדקס 0
    printf("-> SUCCESS: Zero radius search isolates the target point.\n\n");

    // --- טסט 4: חיפוש נקודה ריקה/רחוקה (eps = 1.0 סביב 100,100) ---
    printf("Test 4: Empty Space Search (EPS = 1.0) around (100.0, 100.0)...\n");
    target_point.vals[0] = 100.0;
    target_point.vals[1] = 100.0;
    eps = 1.0;
    count = 0;
    kd_tree_find_neighbors(root, &target_point, eps, &data, neighbor_indexes, &count);
    printf("Found %d neighbors. Expected: 0.\n", count);
    assert(count == 0);
    printf("-> SUCCESS: Empty space search correctly returns 0.\n\n");

    // --- ניקוי זיכרון ---
    printf("Cleaning up memory...\n");
    kd_tree_free(root);
    free(data.arr);
    printf("-> SUCCESS: Memory freed without crashes.\n\n");

    printf("=== All KD-Tree Tests Passed Successfully! Ready for COMMIT. ===\n");
}

int main() {
    run_kdtree_integration_test();
    return 0;
}