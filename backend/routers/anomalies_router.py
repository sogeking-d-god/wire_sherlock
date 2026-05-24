from fastapi import APIRouter, Depends, HTTPException

from api.python.c_schemas import AnomalyBundle, AnomalyClusterResult, ClusterRequest
from api.python.session_wrapper import WireSherlockSession
from backend.sessions.dependencies import get_user_session

from .endpoints import AnomaliesEndpoints

router = APIRouter(prefix=AnomaliesEndpoints.PREFIX, tags=["Anomalies"])


@router.get(AnomaliesEndpoints.GENERATE, response_model=AnomalyBundle)
async def generate_anomalies(
    metric_mask: int,
    z_sensitivity: float | None = None,
    session: WireSherlockSession = Depends(get_user_session),
) -> AnomalyBundle:
    try:
        return await session.generate_anomalies(metric_mask, z_sensitivity)
    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@router.post(AnomaliesEndpoints.CLUSTER, response_model=AnomalyClusterResult)
async def cluster_anomalies(
    body: ClusterRequest,
    session: WireSherlockSession = Depends(get_user_session),
) -> AnomalyClusterResult:
    try:
        cfg_dict = body.cfg.model_dump(exclude_none=True) if body.cfg else None
        return await session.cluster_anomalies(
            metric_mask=body.metric_mask,
            macro_ids=body.macro_ids,
            micro_ids=body.micro_ids,
            cfg=cfg_dict,
        )
    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
