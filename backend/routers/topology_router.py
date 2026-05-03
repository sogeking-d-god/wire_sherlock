from fastapi import APIRouter, HTTPException
from backend.services.engine_service import get_raw_analysis
from backend.services.data_builder import build_topology_data
from backend.models.responses import TopologyResponse

router = APIRouter(prefix="/api/topology", tags=["Topology"])

@router.get("/analyze", response_model=TopologyResponse)
async def analyze_and_get_topology(pcap_path: str):
    try:
        # 1. try to get the raw analysis results from the engine service (in the future add a check if we saved the results in the db already)
        raw_json = get_raw_analysis(pcap_path)

        # 2. build the topology data from the raw JSON using the data builder service for the frontend
        topology = build_topology_data(raw_json)

        return topology

    except FileNotFoundError as e:
        raise HTTPException(status_code=404, detail=str(e))
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Analysis failed: {str(e)}")