from fastapi import APIRouter, Depends, HTTPException

from api.python.c_schemas import HttpAttackReport
from api.python.session_wrapper import WireSherlockSession
from backend.sessions.dependencies import get_user_session

from .endpoints import HttpAttacksEndpoints

router = APIRouter(prefix=HttpAttacksEndpoints.PREFIX, tags=["HTTP Attacks"])


@router.get(HttpAttacksEndpoints.ATTACKS, response_model=HttpAttackReport)
async def get_http_attacks(
    session: WireSherlockSession = Depends(get_user_session),
) -> HttpAttackReport:
    try:
        return await session.analyze_http_advanced()
    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
