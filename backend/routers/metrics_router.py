from fastapi import APIRouter, HTTPException
from backend.models.responses import MetricResponse
from backend.services.data_builder import format_metric_for_frontend
from backend import session_manager

from .endpoints import MetricsEndpoints
router = APIRouter(prefix=MetricsEndpoints.PREFIX, tags=["Metrics"])

@router.get(MetricsEndpoints.GET_METRIC, response_model=MetricResponse)
async def get_traffic_metrics(metric_id: int):
    # 1. Retrieve the active session from the manager
    current_session = session_manager.manager.get_session()

    if current_session is None:
        raise HTTPException(status_code=400, detail="Session not initialized. Please call initialize_default first.")

    try:
        # 2. Get the raw metric bins from the session
        raw_metrics = current_session.get_metric_bins(metric_id)

        # 3. Format the metrics for the frontend
        return format_metric_for_frontend(metric_id, raw_metrics)
    except Exception as e:
        print(f"Metrics Error: {e}")
        raise HTTPException(status_code=500, detail=str(e))