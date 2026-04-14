#include "pelt_dbscan.h"

#include "dbscan.h"
#include "math.h"


/**
 * @brief the function converts the list of segments found by PELT into a geometry of points for DBSCAN,
 * by normalizing and weighting the time, z-score and ssmd values of each segment.
 *
 * @param list The list of segments found by PELT.
 * @param total_duration The total duration of the PCAP, used for normalizing the time dimension.
 * @param sensitivity The sensitivity parameter used for capping and normalizing the z-score and ssmd dimensions.
 * @param out_geometry Pointer to a points_arr_t struct that will be populated with the resulting geometry. The caller is responsible for freeing the allocated points array.
 * @return int 0 on success, -1 on memory failure or invalid input.
 */
static int dbscan_prepare_pelt_data(pelt_segments_list_t *list, uint64_t total_duration,
                             double sensitivity, points_arr_t *out_geometry)
{
    int status = 0;
    int n;

    double norm_time;
    double norm_z;

    double ssmd;
    double norm_ssmd;
    double var_sum;

    double EPSILON = 1e-10; // Small value to prevent division by zero in SSMD calculation

    pelt_segment_t *seg;
    pelt_segment_t *prev_seg;
    point_t *points = NULL;

    if(sensitivity <= 0)
    {
        sensitivity = 10.0; // Default sensitivity if invalid value provided
    }

    if (list && list->count > 0 && total_duration > 0 && out_geometry)
    {
        n = list->count;
        points = (point_t *)malloc(sizeof(point_t) * n);

        if (points)
        {
            for (int i = 0; i < n; i++)
            {
                seg = &list->segments[i];

                // Min max scaling of the time val
                norm_time = ((double)seg->start / (double)total_duration) * TIME_DIM_WEIGHT;

                // Capping and normalization of the Z-Score val
                norm_z = (seg->z_score > 0) ? seg->z_score : -seg->z_score;

                if (norm_z >= sensitivity)
                {
                    norm_z = 1;
                }
                else
                {
                    norm_z = norm_z / sensitivity;
                }
                norm_z = norm_z * Z_SCORE_DIM_WEIGHT;

                // Calculating SSMD
                ssmd = 0;
                norm_ssmd = 0;

                if (i > 0)
                {
                    prev_seg = &list->segments[i - 1];
                    var_sum = seg->variance + prev_seg->variance;

                    if (var_sum > EPSILON)
                    {
                        ssmd = (seg->mean - prev_seg->mean) / sqrt(var_sum);
                    }
                    else if (seg->mean != prev_seg->mean)
                    {
                        // Edge case: Both variances are 0, but there is a jump in the mean.
                        // We set it to max sensitivity to register as a harsh shift.
                        ssmd = sensitivity;
                    }
                }

                // Capping and normalization of the SSMD val
                norm_ssmd = fabs(ssmd);
                if (norm_ssmd >= sensitivity)
                {
                    norm_ssmd = 1.0;
                }
                else
                {
                    norm_ssmd = norm_ssmd / sensitivity;
                }
                norm_ssmd = norm_ssmd * SSMD_DIM_WIEGHT;

                points[i].vals[DIM_TIME] = norm_time;
                points[i].vals[DIM_Z_SCORE] = norm_z;
                points[i].vals[DIM_SSMD] = norm_ssmd;

                points[i].original_index = i;
                points[i].cluster_id = UNCLASSIFIED;
            }

            out_geometry->arr = points;
            out_geometry->len = n;
            out_geometry->dim_count = MAX_DIM_COUNT;
        }
        else
        {
            printf("error: malloc failed in dbscan_prepare_pelt_data!\n");
            status = -1;
        }
    }
    else
    {
        status = -1;
    }

    return status;
}

int pelt_dbscan_analyze_macro(pelt_segments_list_t *list, uint64_t total_duration,
                              double sensitivity, int min_pts, int **out_labels, double *out_eps)
{
    int status = -1;
    double calculated_eps = 0.0;
    points_arr_t geometry;
    int *labels = NULL;

    if (list && out_labels)
    {
        if (dbscan_prepare_pelt_data(list, total_duration, sensitivity, &geometry) == 0)
        {
            status = dbscan_process_auto(&geometry, min_pts, &calculated_eps);

            if (status >= 0)
            {
                labels = (int *)malloc(sizeof(int) * geometry.len);
                if (labels)
                {
                    for (int i = 0; i < geometry.len; i++)
                    {
                        labels[i] = geometry.arr[i].cluster_id;
                    }
                    *out_labels = labels;

                    if (out_eps)
                    {
                        *out_eps = calculated_eps;
                    }
                }
                else
                {
                    printf("error: malloc failed for labels in pelt_dbscan_analyze_macro!\n");
                    status = -1;
                }
            }
            free(geometry.arr);
        }
    }
    return status;
}