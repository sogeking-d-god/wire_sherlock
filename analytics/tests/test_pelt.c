#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pelt.h"

void generate_test_data(time_series_t *ts) {
    ts->count = 100;
    ts->data = (double*)malloc(ts->count * sizeof(double));
    srand(time(NULL)); // אתחול מספרים אקראיים לפי הזמן

    for (int i = 0; i < ts->count; i++) {
        if (i < 40) {
            ts->data[i] = 10.0 + (rand() % 2);
        } else if (i < 80) {
            ts->data[i] = 50.0 + (rand() % 5);
        } else {
            ts->data[i] = 15.0 + (rand() % 2);
        }
    }
}

int main() {
    time_series_t ts;
    pelt_segments_list_t result;
    double penalty;

    generate_test_data(&ts);

    // 1. חישוב קנס
    penalty = pelt_calculate_penalty_BIC(ts.count, 1);
    printf("Running PELT with N=%lu, Penalty=%.2f\n", (unsigned long)ts.count, penalty);
    printf("-------------------------------------------\n");

    // 2. הרצת האלגוריתם - שים לב לשינוי בקבלת הערך
    result = pelt_detect_changepoints(&ts, penalty);

    if (result.segments == NULL) {
        printf("PELT failed or no segments found.\n");
        free(ts.data);
        return 1;
    }

    // 3. הדפסת המקטעים שהתגלו
    printf("Detected %d segments:\n", result.count);
    for (int i = 0; i < result.count; i++) {
        printf("Segment %d: indexes [%lu - %lu], Average: %.2f, Variance: %.2f\n",
               i,
               (unsigned long)result.segments[i].start,
               (unsigned long)result.segments[i].end,
               result.segments[i].mean,
               result.segments[i].variance);

        // אם זה לא המקטע הראשון, סימן שיש כאן נקודת שינוי בתחילתו
        if (i > 0) {
            printf("  ^^ Changepoint detected at index %lu\n", (unsigned long)result.segments[i].start);
        }
    }

    // ניקוי זיכרון - חשוב מאוד!
    free(ts.data);
    free(result.segments); // משחררים את המערך שהוקצה בתוך pelt_reconstruct_segments

    return 0;
}