from fastapi import APIRouter, Depends, HTTPException

from api.python.c_ipc_manager import MetricType
from api.python.session_wrapper import WireSherlockSession
from backend.models.responses import MetricResponse
from backend.services.data_builder import format_metric_for_frontend
from backend.sessions.dependencies import get_user_session

from .endpoints import MetricsEndpoints

router = APIRouter(prefix=MetricsEndpoints.PREFIX, tags=["Metrics"])


@router.get(MetricsEndpoints.GET_METRIC, response_model=MetricResponse)
async def get_traffic_metrics(
    metric_id: int,
    session: WireSherlockSession = Depends(get_user_session),
):
    try:
        metric_enum = MetricType(metric_id)
    except ValueError:
        raise HTTPException(status_code=400, detail=f"Unknown metric_id: {metric_id}")

    try:
        raw_metrics = await session.get_metric_bins_async(metric_enum)
        return format_metric_for_frontend(metric_id, raw_metrics)
    except HTTPException:
        raise
    except Exception as e:
        print(f"Metrics Error: {e}")
        raise HTTPException(status_code=500, detail=str(e))
