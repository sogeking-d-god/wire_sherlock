from typing import Optional

from pydantic import BaseModel, Field


class ChatRequest(BaseModel):
    prompt: str = Field(..., min_length=1, max_length=4000)
    max_tool_hops: Optional[int] = Field(default=None, ge=1, le=10)


class ChatToolCallEvent(BaseModel):
    """Payload of an SSE `tool_call` event — emitted *before* the C-engine call
    so the frontend can render a status spinner during inference pauses."""

    name: str
    arguments: dict
    status: str  # "started" | "completed" | "failed"
    message: Optional[str] = None


class ChatTokenEvent(BaseModel):
    """Payload of an SSE `token` event — a streamed content delta from the LLM."""

    delta: str


class ChatDoneEvent(BaseModel):
    """Payload of the terminating SSE `done` event."""

    reason: str  # "complete" | "tool_hop_limit" | "error"
    detail: Optional[str] = None
