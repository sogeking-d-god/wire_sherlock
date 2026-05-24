from fastapi import APIRouter, Depends, HTTPException

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
