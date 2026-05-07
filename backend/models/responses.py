from pydantic import BaseModel
from typing import List, Dict, Optional, Union

class MacNodeInfo(BaseModel):
    mac: str
    packets_at_node: float
    bytes_at_node: float

class NodeDetails(BaseModel):
    id: str
    label: str
    ip: str
    packets: float
    bytes: float
    macs: List[MacNodeInfo] = []
class SessionDetails(BaseModel):
    id: str
    src_ip: str
    dst_ip: str
    src_port: int
    dst_port: int
    protocol: Union[int, str]
    start_time: float
    end_time: float
    packets: Union[int, float]
    bytes: Union[int, float]
    start_status: str
    end_status: str
class GlobalMacDetails(BaseModel):
    mac: str
    total_packets: float
    total_bytes: float
    associated_ips: List[str]

class TopologyResponse(BaseModel):
    macs: List[GlobalMacDetails]
    nodes: List[NodeDetails]
    links: List[SessionDetails]