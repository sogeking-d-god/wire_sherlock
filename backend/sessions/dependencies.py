from fastapi import Depends, HTTPException, status

from api.python.session_wrapper import WireSherlockSession
from backend.dependencies import get_current_user
from backend.sessions.session_manager import manager


async def get_user_session(
    user_id: str = Depends(get_current_user),
) -> WireSherlockSession:
    sess = manager.get(user_id)
    if sess is None:
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail="No active PCAP — call /api/pcap/select first",
        )
    sess.touch()
    return sess
