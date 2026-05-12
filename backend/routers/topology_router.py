from fastapi import APIRouter, HTTPException
from backend.services.data_builder import build_topology_data
from backend.models.responses import TopologyResponse
# We need to import the session manager to access the global session
from backend import session_manager
from .endpoints import TopologyEndpoints

router = APIRouter(prefix=TopologyEndpoints.PREFIX, tags=["Topology"])

@router.get(TopologyEndpoints.ANALYZE) # This endpoint will trigger the analysis and return the topology data
async def analyze_and_get_topology():
    # 1. Retrieve the active session from the manager
    current_session = session_manager.manager.get_session()

    if not current_session:
        # If there's no active session, we can't proceed with analysis
        raise HTTPException(status_code=400, detail="Manager has no active session. Please call /initialize_default first.")

    try:
        # 2. Run the analysis using the session and get the raw JSON output
        raw_json = current_session.run_analysis()
        return build_topology_data(raw_json)
    except Exception as e:
        print(f"Error in Topology: {e}")
        raise HTTPException(status_code=500, detail=str(e))