class PCAPEndpoints:
    PREFIX = "/api/pcap"
    SELECT = "/select/{file_id}"
    ACTIVE_SESSION = "/active-session"
    FILES = "/files"
    UPLOAD = "/upload"

class TopologyEndpoints:
    PREFIX = "/api/topology"
    ANALYZE = "/analyze"

class MetricsEndpoints:
    PREFIX = "/api/metrics"
    GET_METRIC = "/{metric_id}"