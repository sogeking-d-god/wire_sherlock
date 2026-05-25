"""POST /api/chat — conversational endpoint.

The user sends a natural-language prompt; we instantiate a WireSherlockAgent
bound to their authenticated session and stream the agent's progress back as
Server-Sent Events (SSE). Three event types:

  event: tool_call   data: {"name": "...", "arguments": {...}, "status": "started"|"completed"|"failed"}
  event: token       data: {"delta": "..."}
  event: done        data: {"reason": "complete"|"tool_hop_limit"|"error", "detail": "..."}

The tool_call "started" frame is emitted *before* the C-engine call so the
frontend can show a status spinner while inference is paused.
"""

from __future__ import annotations

import asyncio
import json
import logging
from typing import AsyncIterator

import ollama
from fastapi import APIRouter, Depends, HTTPException
from fastapi.responses import StreamingResponse

from api.python.session_wrapper import WireSherlockSession
from backend.dependencies import get_current_user
from backend.models.chat import ChatRequest
from backend.services.llm_service import OLLAMA_HOST, WireSherlockAgent
from backend.sessions.dependencies import get_user_session

from .endpoints import ChatEndpoints

logger = logging.getLogger(__name__)

router = APIRouter(prefix=ChatEndpoints.PREFIX, tags=["Chat"])

# Liveness probe — if Ollama hasn't finished `ollama pull` yet, fail the chat
# request fast with a clear 503 instead of letting the SSE stream hang for
# minutes. 3 attempts × 5 s timeout + 5 s backoff hides a cold start of up to
# ~15 s while keeping the worst-case rejection bounded.
_LIVENESS_TIMEOUT_S = 5.0
_LIVENESS_RETRY_BACKOFF_S = 5.0
_LIVENESS_MAX_ATTEMPTS = 3


async def _wait_for_ollama() -> bool:
    probe = ollama.AsyncClient(host=OLLAMA_HOST, timeout=_LIVENESS_TIMEOUT_S)
    for attempt in range(1, _LIVENESS_MAX_ATTEMPTS + 1):
        try:
            await probe.list()
            return True
        except Exception as e:  # noqa: BLE001
            logger.info(
                "Ollama liveness probe %d/%d failed: %s",
                attempt, _LIVENESS_MAX_ATTEMPTS, e,
            )
            if attempt < _LIVENESS_MAX_ATTEMPTS:
                await asyncio.sleep(_LIVENESS_RETRY_BACKOFF_S)
    return False


def _format_sse(event: str, data: dict) -> str:
    return f"event: {event}\ndata: {json.dumps(data, ensure_ascii=False)}\n\n"


async def _sse_wrap(agent_stream: AsyncIterator[dict]) -> AsyncIterator[str]:
    try:
        async for frame in agent_stream:
            yield _format_sse(frame["event"], frame["data"])
    except Exception as e:
        logger.exception("Agent stream crashed")
        yield _format_sse("done", {"reason": "error", "detail": str(e)})


@router.post(ChatEndpoints.CHAT)
async def chat(
    body: ChatRequest,
    user_id: str = Depends(get_current_user),
    session: WireSherlockSession = Depends(get_user_session),
) -> StreamingResponse:
    if not body.prompt.strip():
        raise HTTPException(status_code=400, detail="Empty prompt")

    if not await _wait_for_ollama():
        raise HTTPException(
            status_code=503,
            detail="LLM backend is still warming up — try again in a moment",
        )

    agent = WireSherlockAgent(
        session=session,
        user_id=user_id,
        max_tool_hops=body.max_tool_hops,
    )
    return StreamingResponse(
        _sse_wrap(agent.stream(body.prompt)),
        media_type="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "X-Accel-Buffering": "no",  # disable proxy buffering for live streams
        },
    )
