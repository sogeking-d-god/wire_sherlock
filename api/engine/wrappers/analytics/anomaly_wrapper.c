#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anomaly_wrapper.h"
#include "ipc_config.h"
#include "ipc_messenger.h"
#include "anomaly_detector.h"
#include "micro_detector.h"
#include "time_series.h"

// ---------------------------------------------------------------------------
// Cache management
// ---------------------------------------------------------------------------

void anomaly_wrapper_free_caches(file_analysis_context_t *core)
{
    if (!core)
    {
        return;
    }
    if (core->anomaly_macro_cache)
    {
        scored_segments_list_t *m = (scored_segments_list_t *)core->anomaly_macro_cache;
        anomaly_scored_segments_free(m);
        free(m);
        core->anomaly_macro_cache = NULL;
    }
    if (core->anomaly_micro_cache)
    {
        micro_events_list_t *m = (micro_events_list_t *)core->anomaly_micro_cache;
        micro_events_list_free(m);
        free(m);
        core->anomaly_micro_cache = NULL;
    }
}

// ---------------------------------------------------------------------------
// Serialization helpers
// ---------------------------------------------------------------------------

static cJSON *serialize_scored_segment(const scored_segment_t *seg, int id)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "id", id);
    cJSON_AddNumberToObject(obj, "metric", (int)seg->metric);
    cJSON_AddNumberToObject(obj, "start_bin", (double)seg->start_bin);
    cJSON_AddNumberToObject(obj, "end_bin", (double)seg->end_bin);
    cJSON_AddNumberToObject(obj, "mean", seg->mean);
    cJSON_AddNumberToObject(obj, "variance", seg->variance);
    cJSON_AddNumberToObject(obj, "ssmd", seg->ssmd);
    cJSON_AddNumberToObject(obj, "z_global", seg->z_global);
    return obj;
}

static cJSON *serialize_micro_event(const micro_event_t *ev, int id)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "id", id);
    cJSON_AddNumberToObject(obj, "metric", (int)ev->metric);
    cJSON_AddNumberToObject(obj, "bin_index", (double)ev->bin_index);
    cJSON_AddNumberToObject(obj, "value", ev->value);
    cJSON_AddNumberToObject(obj, "z_sliding", ev->z_sliding);
    return obj;
}

static cJSON *serialize_macro_cluster(const macro_cluster_t *c, const int *remap)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "cluster_id", c->cluster_id);
    cJSON_AddNumberToObject(obj, "time_start_bin", (double)c->time_start_bin);
    cJSON_AddNumberToObject(obj, "time_end_bin", (double)c->time_end_bin);
    cJSON_AddNumberToObject(obj, "max_abs_ssmd", c->max_abs_ssmd);
    cJSON_AddNumberToObject(obj, "max_abs_z", c->max_abs_z);

    cJSON *members = cJSON_CreateArray();
    for (int k = 0; k < c->member_count; k++)
    {
        int filtered_idx = c->member_indexes[k];
        int original_id = remap ? remap[filtered_idx] : filtered_idx;
        cJSON_AddItemToArray(members, cJSON_CreateNumber((double)original_id));
    }
    cJSON_AddItemToObject(obj, "member_indexes", members);
    return obj;
}

static cJSON *serialize_micro_burst(const micro_burst_t *b, const int *remap)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "cluster_id", b->cluster_id);
    cJSON_AddNumberToObject(obj, "bin_start", (double)b->bin_start);
    cJSON_AddNumberToObject(obj, "bin_end", (double)b->bin_end);

    cJSON *members = cJSON_CreateArray();
    for (int k = 0; k < b->member_count; k++)
    {
        int filtered_idx = b->member_indexes[k];
        int original_id = remap ? remap[filtered_idx] : filtered_idx;
        cJSON_AddItemToArray(members, cJSON_CreateNumber((double)original_id));
    }
    cJSON_AddItemToObject(obj, "member_indexes", members);
    return obj;
}

// ---------------------------------------------------------------------------
// Config helpers
// ---------------------------------------------------------------------------

static void apply_cfg_overrides(cJSON *cfg_obj, anomaly_config_t *cfg)
{
    if (!cfg_obj || !cJSON_IsObject(cfg_obj))
    {
        return;
    }
    cJSON *item;

    item = cJSON_GetObjectItem(cfg_obj, "k_ssmd");
    if (cJSON_IsNumber(item)) cfg->k_ssmd = item->valuedouble;

    item = cJSON_GetObjectItem(cfg_obj, "k_z");
    if (cJSON_IsNumber(item)) cfg->k_z = item->valuedouble;

    item = cJSON_GetObjectItem(cfg_obj, "w_time");
    if (cJSON_IsNumber(item)) cfg->w_time = item->valuedouble;

    item = cJSON_GetObjectItem(cfg_obj, "w_ssmd");
    if (cJSON_IsNumber(item)) cfg->w_ssmd = item->valuedouble;

    item = cJSON_GetObjectItem(cfg_obj, "w_z");
    if (cJSON_IsNumber(item)) cfg->w_z = item->valuedouble;

    item = cJSON_GetObjectItem(cfg_obj, "macro_min_pts");
    if (cJSON_IsNumber(item)) cfg->macro_min_pts = item->valueint;

    item = cJSON_GetObjectItem(cfg_obj, "ewma_alpha");
    if (cJSON_IsNumber(item)) cfg->ewma_alpha = item->valuedouble;

    item = cJSON_GetObjectItem(cfg_obj, "micro_z_threshold");
    if (cJSON_IsNumber(item)) cfg->micro_z_threshold = item->valuedouble;

    item = cJSON_GetObjectItem(cfg_obj, "micro_min_pts");
    if (cJSON_IsNumber(item)) cfg->micro_min_pts = item->valueint;

    item = cJSON_GetObjectItem(cfg_obj, "micro_eps_bins");
    if (cJSON_IsNumber(item)) cfg->micro_eps_bins = item->valueint;
}

static double pcap_duration_seconds(const file_analysis_context_t *core)
{
    double start = core->start_ts.tv_sec + core->start_ts.tv_usec / 1e6;
    double end = core->end_ts.tv_sec + core->end_ts.tv_usec / 1e6;
    double dur = end - start;
    return dur > 0.0 ? dur : 1.0;
}

// ---------------------------------------------------------------------------
// cmd_generate_anomalies
// ---------------------------------------------------------------------------

void handle_generate_anomalies_request(int client_sock, file_analysis_context_t *core, cJSON *request)
{
    if (!core || !core->bin_manager)
    {
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("No analysis context — call cmd_start first"));
        return;
    }

    cJSON *mask_item = cJSON_GetObjectItem(request, "metric_mask");
    if (!cJSON_IsNumber(mask_item))
    {
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("Missing or invalid metric_mask"));
        return;
    }
    uint32_t metric_mask = (uint32_t)mask_item->valuedouble;

    cJSON *z_item = cJSON_GetObjectItem(request, "z_sensitivity");
    double z_sensitivity = (cJSON_IsNumber(z_item))
                               ? z_item->valuedouble
                               : ANOMALY_DEFAULT_Z_SENSITIVITY;

    anomaly_config_t cfg;
    anomaly_config_defaults(&cfg);
    apply_cfg_overrides(cJSON_GetObjectItem(request, "cfg"), &cfg);

    // Drop any prior caches BEFORE running so we never leak on re-invoke.
    anomaly_wrapper_free_caches(core);

    scored_segments_list_t *macro = calloc(1, sizeof(scored_segments_list_t));
    micro_events_list_t *micro = calloc(1, sizeof(micro_events_list_t));
    if (!macro || !micro)
    {
        free(macro);
        free(micro);
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("Out of memory"));
        return;
    }
    anomaly_scored_segments_init(macro);

    int macro_ok = anomaly_generate_macro_segments(core->bin_manager, metric_mask, z_sensitivity, macro);
    if (!macro_ok)
    {
        anomaly_scored_segments_free(macro);
        free(macro);
        free(micro);
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("anomaly_generate_macro_segments failed"));
        return;
    }

    int micro_ok = anomaly_generate_micro_events(core->bin_manager, metric_mask, &cfg, micro);
    if (!micro_ok)
    {
        anomaly_scored_segments_free(macro);
        free(macro);
        micro_events_list_free(micro);
        free(micro);
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("anomaly_generate_micro_events failed"));
        return;
    }

    // Install caches BEFORE serialization so even a partial JSON write leaves
    // the caches in a queryable state for a subsequent cmd_cluster_anomalies.
    core->anomaly_macro_cache = macro;
    core->anomaly_micro_cache = micro;

    cJSON *data = cJSON_CreateObject();
    cJSON *macro_arr = cJSON_CreateArray();
    for (int i = 0; i < macro->count; i++)
    {
        cJSON_AddItemToArray(macro_arr, serialize_scored_segment(&macro->items[i], i));
    }
    cJSON *micro_arr = cJSON_CreateArray();
    for (int i = 0; i < micro->count; i++)
    {
        cJSON_AddItemToArray(micro_arr, serialize_micro_event(&micro->items[i], i));
    }
    cJSON_AddItemToObject(data, "macro_segments", macro_arr);
    cJSON_AddItemToObject(data, "micro_events", micro_arr);
    cJSON_AddNumberToObject(data, "metric_mask", (double)metric_mask);

    send_api_response(client_sock, STATUS_SUCCESS, data);
}

// ---------------------------------------------------------------------------
// cmd_cluster_anomalies
// ---------------------------------------------------------------------------

static int *build_filtered_macro(const scored_segments_list_t *cache,
                                 cJSON *id_array,
                                 scored_segments_list_t *out_filtered)
{
    int n_ids = cJSON_IsArray(id_array) ? cJSON_GetArraySize(id_array) : 0;
    if (n_ids <= 0)
    {
        out_filtered->items = NULL;
        out_filtered->count = 0;
        return NULL;
    }

    out_filtered->items = calloc(n_ids, sizeof(scored_segment_t));
    int *remap = calloc(n_ids, sizeof(int));
    if (!out_filtered->items || !remap)
    {
        free(out_filtered->items);
        free(remap);
        out_filtered->items = NULL;
        out_filtered->count = 0;
        return NULL;
    }

    int filtered_count = 0;
    for (int i = 0; i < n_ids; i++)
    {
        cJSON *item = cJSON_GetArrayItem(id_array, i);
        if (!cJSON_IsNumber(item))
        {
            continue;
        }
        int id = item->valueint;
        // Hardening Constraint #1: range-check every id before subscripting.
        if (id < 0 || id >= cache->count)
        {
            fprintf(stderr, "[anomaly_wrapper] macro_id %d out of range [0,%d) — skipped\n",
                    id, cache->count);
            continue;
        }
        out_filtered->items[filtered_count] = cache->items[id];
        remap[filtered_count] = id;
        filtered_count++;
    }
    out_filtered->count = filtered_count;
    return remap;
}

static int *build_filtered_micro(const micro_events_list_t *cache,
                                 cJSON *id_array,
                                 micro_events_list_t *out_filtered)
{
    int n_ids = cJSON_IsArray(id_array) ? cJSON_GetArraySize(id_array) : 0;
    if (n_ids <= 0)
    {
        out_filtered->items = NULL;
        out_filtered->count = 0;
        return NULL;
    }

    out_filtered->items = calloc(n_ids, sizeof(micro_event_t));
    int *remap = calloc(n_ids, sizeof(int));
    if (!out_filtered->items || !remap)
    {
        free(out_filtered->items);
        free(remap);
        out_filtered->items = NULL;
        out_filtered->count = 0;
        return NULL;
    }

    int filtered_count = 0;
    for (int i = 0; i < n_ids; i++)
    {
        cJSON *item = cJSON_GetArrayItem(id_array, i);
        if (!cJSON_IsNumber(item))
        {
            continue;
        }
        int id = item->valueint;
        if (id < 0 || id >= cache->count)
        {
            fprintf(stderr, "[anomaly_wrapper] micro_id %d out of range [0,%d) — skipped\n",
                    id, cache->count);
            continue;
        }
        out_filtered->items[filtered_count] = cache->items[id];
        remap[filtered_count] = id;
        filtered_count++;
    }
    out_filtered->count = filtered_count;
    return remap;
}

void handle_cluster_anomalies_request(int client_sock, file_analysis_context_t *core, cJSON *request)
{
    // Hardening Constraint #1: refuse the call if the user skipped generation.
    if (!core || !core->anomaly_macro_cache || !core->anomaly_micro_cache)
    {
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("Anomaly cache is uninitialized. You must execute anomaly generation before running clustering."));
        return;
    }

    cJSON *mask_item = cJSON_GetObjectItem(request, "metric_mask");
    if (!cJSON_IsNumber(mask_item))
    {
        send_api_response(client_sock, STATUS_ERROR,
                          cJSON_CreateString("Missing or invalid metric_mask"));
        return;
    }
    uint32_t metric_mask = (uint32_t)mask_item->valuedouble;

    anomaly_config_t cfg;
    anomaly_config_defaults(&cfg);
    apply_cfg_overrides(cJSON_GetObjectItem(request, "cfg"), &cfg);

    int bin_size_ms = core->bin_manager ? core->bin_manager->bin_size : DEFAULT_BIN_SIZE;
    double duration_s = pcap_duration_seconds(core);

    scored_segments_list_t *macro_cache = (scored_segments_list_t *)core->anomaly_macro_cache;
    micro_events_list_t *micro_cache = (micro_events_list_t *)core->anomaly_micro_cache;

    // ---- Macro path ----
    scored_segments_list_t filtered_macro = {0};
    int *macro_remap = build_filtered_macro(
        macro_cache,
        cJSON_GetObjectItem(request, "macro_ids"),
        &filtered_macro);

    macro_clusters_list_t out_macro = {0};
    if (filtered_macro.count > 0)
    {
        int ok = anomaly_cluster_macro_segments(&filtered_macro, metric_mask, bin_size_ms, duration_s, &cfg, &out_macro);
        if (!ok)
        {
            free(filtered_macro.items);
            free(macro_remap);
            send_api_response(client_sock, STATUS_ERROR,
                              cJSON_CreateString("anomaly_cluster_macro_segments failed"));
            return;
        }
    }

    // ---- Micro path ----
    micro_events_list_t filtered_micro = {0};
    int *micro_remap = build_filtered_micro(
        micro_cache,
        cJSON_GetObjectItem(request, "micro_ids"),
        &filtered_micro);

    micro_bursts_list_t out_bursts = {0};
    if (filtered_micro.count > 0)
    {
        int ok = anomaly_cluster_micro_events(&filtered_micro, metric_mask, &cfg, &out_bursts);
        if (!ok)
        {
            anomaly_macro_clusters_free(&out_macro);
            free(filtered_macro.items);
            free(macro_remap);
            free(filtered_micro.items);
            free(micro_remap);
            send_api_response(client_sock, STATUS_ERROR,
                              cJSON_CreateString("anomaly_cluster_micro_events failed"));
            return;
        }
    }

    // ---- Serialize with id remap (Hardening Constraint #2) ----
    cJSON *data = cJSON_CreateObject();

    cJSON *macro_arr = cJSON_CreateArray();
    for (int i = 0; i < out_macro.count; i++)
    {
        cJSON_AddItemToArray(macro_arr, serialize_macro_cluster(&out_macro.items[i], macro_remap));
    }
    cJSON_AddItemToObject(data, "macro_clusters", macro_arr);

    cJSON *micro_arr = cJSON_CreateArray();
    for (int i = 0; i < out_bursts.count; i++)
    {
        cJSON_AddItemToArray(micro_arr, serialize_micro_burst(&out_bursts.items[i], micro_remap));
    }
    cJSON_AddItemToObject(data, "micro_bursts", micro_arr);

    cJSON_AddNumberToObject(data, "macro_filtered_count", filtered_macro.count);
    cJSON_AddNumberToObject(data, "micro_filtered_count", filtered_micro.count);

    send_api_response(client_sock, STATUS_SUCCESS, data);

    // ---- Cleanup ----
    anomaly_macro_clusters_free(&out_macro);
    free(filtered_macro.items);
    free(macro_remap);
    // micro_bursts free helper:
    for (int i = 0; i < out_bursts.count; i++)
    {
        free(out_bursts.items[i].member_indexes);
    }
    free(out_bursts.items);
    free(filtered_micro.items);
    free(micro_remap);
}
