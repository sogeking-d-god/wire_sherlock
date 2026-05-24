#include "pelt.h"

/**
 * @brief Calculates L2 cost for a segment using Prefix Sums in O(1).
 * Formula: SumSq - (Sum^2 / N).
 *
 * STATISTICAL CONTEXT:
 *      L2 (Least Squares) is used when we assume the data follows a Normal (Gaussian) distribution and we are looking for changes in the MEAN.
 *
 * @param ps Pointer to precalculated prefix sums.
 * @param start Start index of the segment.
 * @param end End index of the segment.
 * @return double The calculated cost (sum of squared deviations).
 */
static double pelt_calculate_cost(prefix_sums_t *ps, int start, int end)
{
    int n = end - start + 1;
    double s = ps->sum[end + 1] - ps->sum[start];
    double s2 = ps->sum_sq[end + 1] - ps->sum_sq[start];

    // L2: square distance from average = SumSq - (Sum^2 / N)
    double cost = s2 - (s * s) / n;
    return (cost < 0) ? 0 : cost;
}

/**
 * @brief Initializes and fills prefix sums for O(1) cost calculations in the future.
 *
 * @param ps Pointer to prefix_sums_t struct to fill.
 * @param elements Pointer to the raw time series data.
 * @return int 1 on success, 0 on memory allocation failure.
 */
static int pelt_fill_in_prefix_sums(prefix_sums_t * ps, time_series_t * elements)
{
    int ret_val = 0;

    ps->sum = (double*)calloc(elements->count + 1, sizeof(double));
    ps->sum_sq = (double*)calloc(elements->count + 1, sizeof(double));

    if(ps->sum && ps->sum_sq)
    {
        ret_val = 1;

        for (int i = 0; i < elements->count; i++)
        {
            ps->sum[i+1] = ps->sum[i] + elements->data[i];
            ps->sum_sq[i+1] = ps->sum_sq[i] + elements->data[i] * elements->data[i];
        }
    }
    return ret_val;
}

/**
 * @brief Updates the list of candidate changepoints (Pruning step).
 * Removes candidates that cannot be optimal based on the pruning condition:
 * F(tau) + Cost(tau, t) + penalty <= F(t)
 *
 * @param ps Pointer to prefix sums.
 * @param F Array of optimal costs up to time t.
 * @param candidates Array of potential changepoint indexes.
 * @param num_candidates Pointer to the number of current candidates (updated in-place).
 * @param t Current time step in the detection loop.
 */
static void pelt_update_candidates(prefix_sums_t *ps, double *F, int *candidates, int *num_candidates, int t)
{
    int next_num = 0;
    int tau;

    for (int i = 0; i < *num_candidates; i++)
    {
        tau = candidates[i];

        // F(tau) + Cost(tau, t) <= F(t)
        if (F[tau] + pelt_calculate_cost(ps, tau, t - 1) <= F[t])
        {
            candidates[next_num++] = tau;
        }
    }
    // add the last point to candidates
    candidates[next_num++] = t;
    *num_candidates = next_num;
}

/**
 * @brief Finds the best previous changepoint (tau) for the current time t.
 * Iterates through candidates to find tau that has minimal cost: F(tau) + Cost(tau, t) + Penalty.
 *
 * @param ps Pointer to prefix sums.
 * @param F Array of optimal costs up to time t.
 * @param candidates Array of potential changepoint indexes.
 * @param num_candidates Number of current candidates.
 * @param t Current time step.
 * @param penalty The penalty value to discourage over-segmentation.
 * @param min_out Output pointer for the minimum cost found.
 * @return int The index of the best changepoint (tau).
 */
static int pelt_find_best_tau(prefix_sums_t *ps, double *F, int *candidates, int num_candidates, int t, double penalty, double *min_out)
{
    int best_tau = 0;
    double min_val = DBL_MAX;
    int tau;
    double current_val;

    for (int i = 0; i < num_candidates; i++)
    {
        tau = candidates[i];
        current_val = F[tau] + pelt_calculate_cost(ps, tau, t - 1) + penalty;

        if (current_val < min_val)
        {
            min_val = current_val;
            best_tau = tau;
        }
    }
    *min_out = min_val;
    return best_tau;
}

/**
 * @brief Reconstructs the segments list by backtracking through last_cp, and prepares output struct.
 *
 * @param ps Prefix sums for O(1) stats calculation.
 * @param last_cp Backtracking array from PELT.
 * @param n Total number of points.
 * @return pelt_segments_list_t The list of reconstructed segments.
 */
static pelt_segments_list_t pelt_reconstruct_segments(prefix_sums_t *ps, int *last_cp, int n)
{
    pelt_segments_list_t list;
    int num_segments;
    int temp_curr;
    int curr;
    int prev;
    int i;
    double segment_len;
    double seg_sum;
    double seg_squered_sum;

    list.segments = NULL;
    list.count = 0;

    // count segments
    num_segments = 0;
    temp_curr = n;
    while (temp_curr > 0)
    {
        temp_curr = last_cp[temp_curr];
        num_segments++;
    }

    // alocate and fill segments with stats: start index, end index, mean, variance that will alow us to calculate z_score and ssmd later on.
    list.segments = (pelt_segment_t*)malloc(num_segments * sizeof(pelt_segment_t));
    if (list.segments)
    {
        list.count = num_segments;
        curr = n;

        // Backtracing
        for (i = num_segments - 1; i >= 0; i--)
        {
            prev = last_cp[curr];
            segment_len = (double)(curr - prev);

            list.segments[i].start = (uint64_t)prev;
            list.segments[i].end = (uint64_t)curr - 1;

            seg_sum = ps->sum[curr] - ps->sum[prev];
            list.segments[i].mean = seg_sum / segment_len;

            seg_squered_sum = ps->sum_sq[curr] - ps->sum_sq[prev];
            list.segments[i].variance = (seg_squered_sum / segment_len) - (list.segments[i].mean * list.segments[i].mean);

            curr = prev;
        }
    }
    else
    {
        printf("error: malloc failed in pelt_reconstruct_segments\n");
    }

    return list;
}

double pelt_calculate_penalty_BIC(uint64_t n, int p)
{
    double ret_val = 0;
    if (n > 1)
    {
        ret_val = p * log((double)n);
    }
    return ret_val;
}

pelt_segments_list_t pelt_detect_changepoints(time_series_t *elements, double penalty)
{
    int n;
    prefix_sums_t ps;
    int helper_ret;
    double *F;
    int *last_cp;
    int *candidates;
    int num_candidates;
    int t;
    pelt_segments_list_t result;

    n = elements->count;
    result.segments = NULL;
    result.count = 0;

    helper_ret = pelt_fill_in_prefix_sums(&ps, elements);
    F = (double*)malloc((n + 1) * sizeof(double));
    last_cp = (int*)malloc((n + 1) * sizeof(int));
    candidates = (int*)malloc((n + 1) * sizeof(int));

    if (!helper_ret || !F || !last_cp || !candidates)
    {
        printf("error: malloc failed in pelt_detect_changepoints\n");
        free(ps.sum);
        free(ps.sum_sq);
        free(F);
        free(last_cp);
        free(candidates);
    }
    else
    {
        F[0] = -penalty;
        num_candidates = 1;
        candidates[0] = 0;

        for (t = 1; t <= n; t++)
        {
            F[t] = 0;
            last_cp[t] = pelt_find_best_tau(&ps, F, candidates, num_candidates, t, penalty, &F[t]);
            pelt_update_candidates(&ps, F, candidates, &num_candidates, t);
        }

        // prepare output struct
        result = pelt_reconstruct_segments(&ps, last_cp, n);

        free(F);
        free(last_cp);
        free(candidates);
        free(ps.sum);
        free(ps.sum_sq);
    }

    return result;
}
