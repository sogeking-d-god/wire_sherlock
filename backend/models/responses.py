from pydantic import BaseModel
from typing import List, Dict, Optional, Union

class NodeDetails(BaseModel):
    id: str
    label: str
    ip: str
    packets: Union[int, float]
    bytes: Union[int, float]
    mac: str = "Unknown"

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

class TopologyResponse(BaseModel):
    nodes: Dict[str, NodeDetails]
    links: List[SessionDetails]