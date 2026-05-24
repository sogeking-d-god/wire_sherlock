#ifndef ANOMALY_WRAPPER_H
#define ANOMALY_WRAPPER_H

#include <cjson/cJSON.h>
#include "parser.h"
#include "anomaly_types.h"

/**
 * @brief IPC handler for cmd_generate_anomalies.
 *
 * Reads metric_mask and optional z_sensitivity from the request,
 * calls anomaly_generate_macro_segments + anomaly_generate_micro_events,
 * installs the freshly generated lists on core->anomaly_macro_cache /
 * core->anomaly_micro_cache (freeing any prior cache), and serializes
 * the lists into a JSON response.
 */
void handle_generate_anomalies_request(int client_sock, file_analysis_context_t *core, cJSON *request);

/**
 * @brief IPC handler for cmd_cluster_anomalies (filtered DBSCAN).
 *
 * Hardening Constraint #1: validates cache pointers before any deref.
 * Hardening Constraint #2: builds filtered sublists from caller-supplied
 * id arrays, runs DBSCAN, and remaps cluster member_indexes back to the
 * ORIGINAL cached ids before serialization.
 */
void handle_cluster_anomalies_request(int client_sock, file_analysis_context_t *core, cJSON *request);

/**
 * @brief Frees both anomaly caches on the core. Idempotent. Called by core_free
 *        and at the top of every cmd_generate_anomalies handler.
 */
void anomaly_wrapper_free_caches(file_analysis_context_t *core);

#endif
