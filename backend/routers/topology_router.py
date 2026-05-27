from fastapi import APIRouter, Depends, HTTPException, Query

from api.python.c_schemas import IpStat
from api.python.session_wrapper import WireSherlockSession
from backend.services.data_builder import build_topology_data
from backend.sessions.dependencies import get_user_session

from .endpoints import TopologyEndpoints

router = APIRouter(prefix=TopologyEndpoints.PREFIX, tags=["Topology"])


@router.get(TopologyEndpoints.ANALYZE)
async def analyze_and_get_topology(
    session: WireSherlockSession = Depends(get_user_session),
):
    try:
        # Analysis is run once during /api/pcap/select; reuse the cached result.
        # If the cache is missing (e.g. session restored without analysis), trigger now.
        if session.analysis_summary is None:
            await session.run_analysis_async()
        return build_topology_data(session.analysis_summary)
    except HTTPException:
        raise
    except Exception as e:
        print(f"Error in Topology: {e}")
        raise HTTPException(status_code=500, detail=str(e))


@router.get(TopologyEndpoints.SUBNET, response_model=list[IpStat])
async def query_subnet(
    subnet: str = Query(..., description="Dotted IPv4 or colon IPv6 base address"),
    prefix_len: int = Query(..., ge=0, le=128),
    ip_type: int = Query(..., ge=4, le=6),
    session: WireSherlockSession = Depends(get_user_session),
):
    if session.analysis_summary is None:
        await session.run_analysis_async()
    try:
        return await session.query_subnet_async(subnet, prefix_len, ip_type)
    except HTTPException:
        raise
    except Exception as e:
        print(f"Error in Subnet query: {e}")
        raise HTTPException(status_code=500, detail=str(e))
