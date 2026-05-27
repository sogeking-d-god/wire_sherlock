class PCAPEndpoints:
    PREFIX = "/api/pcap"
    SELECT = "/select/{file_id}"
    ACTIVE_SESSION = "/active-session"
    FILES = "/files"
    UPLOAD = "/upload"

class TopologyEndpoints:
    PREFIX = "/api/topology"
    ANALYZE = "/analyze"
    SUBNET = "/subnet"

class MetricsEndpoints:
    PREFIX = "/api/metrics"
    GET_METRIC = "/{metric_id}"

class AnomaliesEndpoints:
    PREFIX = "/api/anomalies"
    GENERATE = "/generate"
    CLUSTER = "/cluster"

class HttpAttacksEndpoints:
    PREFIX = "/api/http"
    ATTACKS = "/attacks"

class ChatEndpoints:
    PREFIX = "/api"
    CHAT = "/chat"
