from pydantic import BaseModel, Field
from typing import List, Optional

# --- L3: IP Stats ---
class IpStat(BaseModel):
    ip: str
    packets: float
    bytes: float

class IpTreeData(BaseModel):
    history: List[IpStat] = []
    count: int = 0

# --- L2: MAC Stats ---
class MacStat(BaseModel):
    mac: str
    packets: float
    bytes: float

    # Optional: will be displayed only if the C engine provides this history
    ipv4_data: Optional[IpTreeData] = None
    ipv6_data: Optional[IpTreeData] = None

# --- L4: Flows & Sessions ---
class FlowKey(BaseModel):
    src_ip: str
    dst_ip: str
    src_port: int
    dst_port: int
    protocol: int
    ip_type: int

class DeviceData(BaseModel):
    packets_sent: int
    bytes_sent: int
class SessionData(BaseModel):
    session_idx: int
    start_state: int
    end_state: int
    start_time: float
    end_time: float
    devices: List[DeviceData]
class FlowData(BaseModel):
    key: FlowKey
    sessions: List[SessionData]

# --- Final Parser Result ---
class ParserResult(BaseModel):
    status: str
    total_packets: int = 0
    total_unique_macs: int = 0
    mac_stats: Optional[List[MacStat]] = None
    global_ipv4_stats: Optional[List[IpStat]] = None
    global_ipv6_stats: Optional[List[IpStat]] = None
    flows: Optional[List[FlowData]] = None

class MetricResult(BaseModel):
    """
    Schema for raw metric data received from the C Engine IPC.
    Used internally between the IPC manager and the Session wrapper.
    """
    metric_id: int
    start_ts: float      # Epoch time of the first packet
    bin_size_ms: int     # Resolution (e.g., 100ms)
    total_bins: int
    data: List[float]    # The actual statistical values (bins)


# ---------------------------------------------------------------------------
# Advanced analytics — schemas for cmd_analyze_http / cmd_generate_anomalies /
# cmd_cluster_anomalies. The C side serializes results to JSON matching these.
# ---------------------------------------------------------------------------

class HttpAttackMatch(BaseModel):
    signature_id: str
    attack_class: str
    match_offset: int
    match_length: int
    matched_bytes: str
    flow_key: Optional[str] = None


class HttpAttackReport(BaseModel):
    match_count: int
    matches: List[HttpAttackMatch] = []


class ScoredSegment(BaseModel):
    id: int
    metric: int
    start_bin: int
    end_bin: int
    mean: float
    variance: float
    ssmd: float
    z_global: float


class MicroEvent(BaseModel):
    id: int
    metric: int
    bin_index: int
    value: float
    z_sliding: float


class AnomalyBundle(BaseModel):
    macro_segments: List[ScoredSegment] = []
    micro_events: List[MicroEvent] = []


class MacroCluster(BaseModel):
    cluster_id: int
    time_start_bin: int
    time_end_bin: int
    member_indexes: List[int] = []
    max_abs_ssmd: float
    max_abs_z: float


class MicroBurst(BaseModel):
    cluster_id: int
    bin_start: int
    bin_end: int
    member_indexes: List[int] = []


class AnomalyClusterResult(BaseModel):
    macro_clusters: List[MacroCluster] = []
    micro_bursts: List[MicroBurst] = []


class AnomalyClusterConfig(BaseModel):
    k_ssmd: Optional[float] = None
    k_z: Optional[float] = None
    w_time: Optional[float] = None
    w_ssmd: Optional[float] = None
    w_z: Optional[float] = None
    macro_min_pts: Optional[int] = None
    micro_min_pts: Optional[int] = None
    micro_eps_bins: Optional[int] = None


class ClusterRequest(BaseModel):
    metric_mask: int
    macro_ids: List[int] = []
    micro_ids: List[int] = []
    cfg: Optional[AnomalyClusterConfig] = None