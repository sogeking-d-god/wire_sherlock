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