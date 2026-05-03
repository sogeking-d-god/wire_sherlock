from pydantic import BaseModel
from typing import List, Dict, Optional

class NodeDetails(BaseModel):
    ip: str
    macs: List[str] = []
    total_packets: int = 0

class SessionDetails(BaseModel):
    id: str
    src_ip: str
    dst_ip: str
    start_time: float
    end_time: float
    protocol: str

class TopologyResponse(BaseModel):
    nodes: Dict[str, NodeDetails]  # IP -> NodeDetails
    links: List[SessionDetails]    # List of sessions representing links between nodes
from pydantic import BaseModel
from typing import List, Dict
