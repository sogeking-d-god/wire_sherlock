"""
Local-LLM agent that drives the per-user C-engine session via direct Python
calls (no loopback HTTP). The LLM (Ollama, default llama3.2:3b) selects tools
via native tool-calling; each tool wrapper summarizes the C-engine output
*before* it is fed back into the model so we stay inside its small context.

Concurrency: each chat request constructs its own WireSherlockAgent bound to
the caller's WireSherlockSession. The session already serializes IPC via
`session.lock`, so multiple parallel chat requests for different users are
safe; multiple in-flight calls for the same user will queue.
"""

from __future__ import annotations

import asyncio
import json
import logging
import os
from typing import Any, AsyncIterator, Awaitable, Callable

import ollama

from api.python.c_ipc_manager import MetricType
from api.python.c_schemas import (
    AnomalyBundle,
    AnomalyClusterResult,
    HttpAttackReport,
)
from api.python.session_wrapper import WireSherlockSession

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Settings — overridable via env so docker-compose can inject them
# ---------------------------------------------------------------------------
OLLAMA_HOST: str = os.environ.get("OLLAMA_HOST", "http://ollama:11434")
OLLAMA_MODEL: str = os.environ.get("OLLAMA_MODEL", "llama3.2:3b")
# Per-chat overall ceiling. 5 minutes is generous on CPU; previously 600 s let
# a single cold request hang a tab for 10 minutes.
OLLAMA_TIMEOUT_S: float = float(os.environ.get("OLLAMA_TIMEOUT_S", "300.0"))
# Warmup uses its own short timeout + retry loop (see warmup_model) so we never
# hold a socket open for the full chat budget while waiting for `ollama pull`.
OLLAMA_WARMUP_TIMEOUT_S: float = float(os.environ.get("OLLAMA_WARMUP_TIMEOUT_S", "20.0"))
OLLAMA_WARMUP_MAX_ATTEMPTS: int = int(os.environ.get("OLLAMA_WARMUP_MAX_ATTEMPTS", "30"))
OLLAMA_WARMUP_BACKOFF_S: float = float(os.environ.get("OLLAMA_WARMUP_BACKOFF_S", "10.0"))
MAX_TOOL_HOPS: int = int(os.environ.get("AGENT_MAX_TOOL_HOPS", "6"))

# Top-K bound applied inside every summarizer.
TOP_K: int = 5

# Mapping the LLM uses for metric_mask bitmasks.
_METRIC_NAMES = {m.value: m.name for m in MetricType}
_METRIC_BIT = {m.name: 1 << m.value for m in MetricType}
_ALL_METRICS_MASK = sum(_METRIC_BIT.values())


# ---------------------------------------------------------------------------
# System prompt — strict instruction layer + few-shot example. Small local
# models sometimes drift from the tool-calling JSON contract, so we reinforce
# explicitly and show one correctly-formatted call.
# ---------------------------------------------------------------------------
_METRIC_LIST_FOR_PROMPT = "\n".join(
    f"  - {name} = bit {val}  (mask value {1 << val})"
    for val, name in _METRIC_NAMES.items()
)

SYSTEM_PROMPT = f"""You are WireSherlock, a network forensic assistant analyzing one PCAP for one user.

Tool-dispatch rules (STRICT — wrong tool = wrong answer):
- Question mentions "HTTP", "SQLi", "XSS", "injection", "attack signature", "payload" → call find_http_attacks. NEVER call find_anomalies for these.
- Question mentions "spike", "burst", "anomaly", "weird traffic", "unusual", "z-score", "outlier" → call find_anomalies with metric_mask={_ALL_METRICS_MASK} (all metrics). NEVER call find_http_attacks for these.
- Optionally group anomaly results with cluster_anomalies using IDs from a previous find_anomalies result.
- Never invent findings. If a tool returns count=0 or an empty list, say so plainly in one sentence and stop.

Field glossary (use EXACTLY these meanings — do not invent acronyms or units):
- z_global (macro segments only): file-wide z-score on a PELT segment — standard deviations from the file-wide mean of that metric. Dimensionless. Higher absolute value = more anomalous segment.
- z_sliding (micro events only): sliding-window EWMA z-score for a single 100 ms bin. Dimensionless. Higher absolute value = more anomalous single-bin spike.
- ssmd (macro segments only): Strictly Standardized Mean Difference between this PELT segment and the previous one — |Δmean| / sqrt(var_i + var_{{i-1}}). Dimensionless effect size of the changepoint. NOT seconds, NOT a delay, NOT an absolute anomaly score. Micro events do NOT have ssmd.
- time_s / t_s: seconds since pcap start. A pair [start, end] for macro segments; a single value for micro events.
- metric names (PACKET_COUNT, BYTE_COUNT, SYN_COUNT, FIN_COUNT, RST_COUNT, ACK_COUNT, PUSH_COUNT): each is a per-100ms-bin packet / byte / flag count.
- attack_class: the signature category label (e.g. SQLi, XSS) — quote it verbatim.

Reply rules (CRITICAL):
- Always provide a short text reply to the user. Never finish a turn with only a tool call and no words.
- After a tool returns, summarize its findings in natural language. Cite the metric name, the time range in seconds, the z-score, or the attack class — whichever the tool actually returned.
- Do not expand abbreviations you are unsure of. If the tool output uses an abbreviation that is not in the glossary above, quote it verbatim without inventing an expansion.
- Keep the final reply under 4 sentences. Do not restate tool JSON verbatim.
"""


# ---------------------------------------------------------------------------
# Summarizers — keep payloads to the model under ~800 tokens / ~3 KB JSON
# ---------------------------------------------------------------------------
def _bin_to_seconds(bin_idx: int, bin_size_ms: int = 100) -> float:
    return round(bin_idx * bin_size_ms / 1000.0, 3)


def _summarize_http(report: HttpAttackReport) -> dict:
    by_class: dict[str, int] = {}
    for m in report.matches:
        by_class[m.attack_class] = by_class.get(m.attack_class, 0) + 1

    top = sorted(report.matches, key=lambda m: m.match_length, reverse=True)[:TOP_K]
    return {
        "count": report.match_count,
        "by_class": by_class,
        "top_5": [
            {
                "signature_id": m.signature_id,
                "attack_class": m.attack_class,
                "flow_key": m.flow_key,
                "matched_bytes_preview": (m.matched_bytes or "")[:80],
            }
            for m in top
        ],
        "hint": "Counts are exhaustive. The top_5 list is the longest matches; this tool is terminal — synthesize your answer now.",
    }


def _summarize_anomalies(bundle: AnomalyBundle) -> dict:
    macro_sorted = sorted(
        bundle.macro_segments, key=lambda s: abs(s.z_global), reverse=True
    )[:TOP_K]
    micro_sorted = sorted(
        bundle.micro_events, key=lambda e: abs(e.z_sliding), reverse=True
    )[:TOP_K]
    return {
        "macro_count": len(bundle.macro_segments),
        "micro_count": len(bundle.micro_events),
        "top_5_macro": [
            {
                "id": s.id,
                "metric": _METRIC_NAMES.get(s.metric, str(s.metric)),
                "time_s": [_bin_to_seconds(s.start_bin), _bin_to_seconds(s.end_bin)],
                "z_global": round(s.z_global, 2),
                "ssmd": round(s.ssmd, 2),
            }
            for s in macro_sorted
        ],
        "top_5_micro": [
            {
                "id": e.id,
                "metric": _METRIC_NAMES.get(e.metric, str(e.metric)),
                "t_s": _bin_to_seconds(e.bin_index),
                "z_sliding": round(e.z_sliding, 2),
            }
            for e in micro_sorted
        ],
        "hint": "Pass the IDs in top_5_macro / top_5_micro to cluster_anomalies to group them temporally.",
    }


def _summarize_clusters(result: AnomalyClusterResult) -> dict:
    clusters = sorted(
        result.macro_clusters, key=lambda c: abs(c.max_abs_z), reverse=True
    )[:TOP_K]
    return {
        "cluster_count": len(result.macro_clusters),
        "micro_burst_count": len(result.micro_bursts),
        "clusters": [
            {
                "cluster_id": c.cluster_id,
                "time_s": [
                    _bin_to_seconds(c.time_start_bin),
                    _bin_to_seconds(c.time_end_bin),
                ],
                "member_count": len(c.member_indexes),
                "max_abs_z": round(c.max_abs_z, 2),
                "max_abs_ssmd": round(c.max_abs_ssmd, 2),
            }
            for c in clusters
        ],
        "hint": "Cluster bounds are inclusive seconds. Raw member IDs are deliberately omitted.",
    }


# ---------------------------------------------------------------------------
# Tool schemas — the JSON-schema dialect Ollama's chat() expects for tools.
# ---------------------------------------------------------------------------
_TOOL_SCHEMAS: list[dict] = [
    {
        "type": "function",
        "function": {
            "name": "find_http_attacks",
            "description": (
                "Scan reassembled HTTP streams for attack signatures "
                "(SQL injection, XSS, command injection, etc.). Returns counts "
                "by attack class and the 5 longest matches."
            ),
            "parameters": {"type": "object", "properties": {}, "required": []},
        },
    },
    {
        "type": "function",
        "function": {
            "name": "find_anomalies",
            "description": (
                "Detect statistically anomalous time bins across selected traffic "
                "metrics. Returns top 5 macro segments and top 5 micro events "
                "ranked by absolute z-score."
            ),
            "parameters": {
                "type": "object",
                "properties": {
                    "metric_mask": {
                        "type": "integer",
                        "description": (
                            "Bitmask of MetricType values. Use 127 to scan all "
                            "(PACKET|BYTE|SYN|FIN|RST|ACK|PUSH)."
                        ),
                    },
                    "z_sensitivity": {
                        "type": ["number", "null"],
                        "description": (
                            "Optional z-score sensitivity threshold. Null = engine default."
                        ),
                    },
                },
                "required": ["metric_mask"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "cluster_anomalies",
            "description": (
                "Group anomalies returned by find_anomalies into temporal clusters. "
                "Pass the IDs from a previous find_anomalies result."
            ),
            "parameters": {
                "type": "object",
                "properties": {
                    "metric_mask": {"type": "integer"},
                    "macro_ids": {
                        "type": "array",
                        "items": {"type": "integer"},
                        "description": "IDs of macro segments to cluster (from find_anomalies.top_5_macro).",
                    },
                    "micro_ids": {
                        "type": "array",
                        "items": {"type": "integer"},
                        "description": "IDs of micro events to cluster (from find_anomalies.top_5_micro).",
                    },
                },
                "required": ["metric_mask", "macro_ids", "micro_ids"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "list_metrics",
            "description": "Return the cheat-sheet of metric names and their bitmask values. No side effects.",
            "parameters": {"type": "object", "properties": {}, "required": []},
        },
    },
]


# ---------------------------------------------------------------------------
# Agent
# ---------------------------------------------------------------------------
ToolImpl = Callable[[dict], Awaitable[dict]]


async def warmup_model() -> None:
    """Pre-load the model into Ollama's RAM so the first /api/chat doesn't pay
    a 30–60 s cold-start tax. Strictly best-effort:

    - Each attempt has a SHORT timeout so we never hold a socket open for the
      full per-chat budget. Retries with linear backoff to cover Ollama's first
      `ollama pull` on a fresh compose-up (model is ~2 GB).
    - On `asyncio.CancelledError` (lifespan shutdown) returns immediately —
      this is what keeps `docker-compose down` from hanging on the warmup task.
    - Never raises — never blocks startup, never blocks shutdown.
    """
    client = ollama.AsyncClient(host=OLLAMA_HOST, timeout=OLLAMA_WARMUP_TIMEOUT_S)
    for attempt in range(1, OLLAMA_WARMUP_MAX_ATTEMPTS + 1):
        try:
            await client.generate(
                model=OLLAMA_MODEL,
                prompt="",
                options={"num_predict": 1},
                keep_alive="30m",
            )
            logger.info("Ollama model %s warmed up on attempt %d", OLLAMA_MODEL, attempt)
            return
        except asyncio.CancelledError:
            logger.info("Ollama warmup cancelled during shutdown")
            raise
        except Exception as e:  # noqa: BLE001 — best-effort
            logger.info(
                "Ollama warmup attempt %d/%d failed (%s) — retrying in %.0fs",
                attempt, OLLAMA_WARMUP_MAX_ATTEMPTS, e, OLLAMA_WARMUP_BACKOFF_S,
            )
            try:
                await asyncio.sleep(OLLAMA_WARMUP_BACKOFF_S)
            except asyncio.CancelledError:
                logger.info("Ollama warmup cancelled during backoff")
                raise
    logger.warning("Ollama warmup gave up after %d attempts", OLLAMA_WARMUP_MAX_ATTEMPTS)


class WireSherlockAgent:
    """One agent per chat request, bound to one authenticated user's session."""

    def __init__(
        self,
        session: WireSherlockSession,
        user_id: str,
        max_tool_hops: int | None = None,
    ):
        self.session = session
        self.user_id = user_id
        self.max_tool_hops = max_tool_hops or MAX_TOOL_HOPS
        self.client = ollama.AsyncClient(host=OLLAMA_HOST, timeout=OLLAMA_TIMEOUT_S)
        self._tools: dict[str, ToolImpl] = {
            "find_http_attacks": self._tool_find_http_attacks,
            "find_anomalies": self._tool_find_anomalies,
            "cluster_anomalies": self._tool_cluster_anomalies,
            "list_metrics": self._tool_list_metrics,
        }

    # -- tool implementations ------------------------------------------------
    async def _tool_find_http_attacks(self, _args: dict) -> dict:
        report = await self.session.analyze_http_advanced()
        return _summarize_http(report)

    async def _tool_find_anomalies(self, args: dict) -> dict:
        metric_mask = int(args.get("metric_mask", _ALL_METRICS_MASK))
        z_sensitivity = args.get("z_sensitivity")
        z_val = float(z_sensitivity) if z_sensitivity is not None else None
        bundle = await self.session.generate_anomalies(metric_mask, z_val)
        return _summarize_anomalies(bundle)

    async def _tool_cluster_anomalies(self, args: dict) -> dict:
        metric_mask = int(args.get("metric_mask", _ALL_METRICS_MASK))
        macro_ids = [int(x) for x in args.get("macro_ids", [])]
        micro_ids = [int(x) for x in args.get("micro_ids", [])]
        result = await self.session.cluster_anomalies(
            metric_mask=metric_mask,
            macro_ids=macro_ids,
            micro_ids=micro_ids,
            cfg=None,
        )
        return _summarize_clusters(result)

    async def _tool_list_metrics(self, _args: dict) -> dict:
        return {
            "metrics": [
                {"name": name, "bit": val, "mask_value": 1 << val}
                for val, name in _METRIC_NAMES.items()
            ],
            "all_mask": _ALL_METRICS_MASK,
        }

    # -- main loop -----------------------------------------------------------
    async def stream(self, user_prompt: str) -> AsyncIterator[dict]:
        """
        Yields dicts shaped as {"event": "token"|"tool_call"|"done", "data": {...}}.
        The router translates each dict into an SSE frame.

        Within one hop we stream Ollama with stream=True so:
          - Plain text answers reach the user token-by-token instead of in one
            batch after a 60-120 s blocking wait on a CPU-only laptop.
          - The SSE connection stays warm while the model is "typing" — no
            silent socket → no client / proxy timeouts.
        Tool-calling turns are not streamable from llama3.2 (the tool_calls
        field only appears on the FINAL chunk), so on those turns we just
        consume the stream silently and act on the assembled tool_calls.
        """
        logger.info(
            "Agent stream starting user=%s prompt=%r",
            self.user_id, user_prompt[:80],
        )
        hops_completed = 0
        messages: list[dict] = [
            {"role": "system", "content": SYSTEM_PROMPT},
            {"role": "user", "content": user_prompt},
        ]

        for _hop in range(self.max_tool_hops + 1):
            hops_completed = _hop + 1
            content_parts: list[str] = []
            tool_calls: list[Any] = []
            saw_any_chunk = False

            try:
                response_iter = await self.client.chat(
                    model=OLLAMA_MODEL,
                    messages=messages,
                    tools=_TOOL_SCHEMAS,
                    stream=True,
                    options={
                        # 0.0 = deterministic tool routing. The 1b model picks
                        # the wrong tool roughly 1 turn in 3 at 0.2; at 0.0 it
                        # follows the dispatch rules in SYSTEM_PROMPT reliably.
                        "temperature": 0.0,
                        # Cap output length so the model doesn't ramble for
                        # minutes on CPU. 256 tokens is plenty for the
                        # "summarize the tool result" reply.
                        "num_predict": 256,
                    },
                    # Pin the model in memory for half an hour so the next
                    # user request doesn't pay the load tax again.
                    keep_alive="30m",
                )
                async for chunk in response_iter:
                    saw_any_chunk = True
                    msg = _get(chunk, "message", {}) or {}
                    delta = _get(msg, "content", "") or ""
                    if delta:
                        content_parts.append(delta)
                        # Stream token deltas to the client immediately so the
                        # user sees progress on slow CPU inference.
                        yield {"event": "token", "data": {"delta": delta}}
                    tc = _get(msg, "tool_calls", None)
                    if tc:
                        # Ollama emits tool_calls only on the terminal chunk —
                        # collect them, don't stream them.
                        tool_calls = list(tc)
            except Exception as e:
                logger.exception("Ollama chat failed")
                logger.info(
                    "Agent stream finished user=%s hops=%d reason=error",
                    self.user_id, hops_completed,
                )
                yield {"event": "done", "data": {"reason": "error", "detail": str(e)}}
                return

            if not saw_any_chunk:
                logger.info(
                    "Agent stream finished user=%s hops=%d reason=empty_stream",
                    self.user_id, hops_completed,
                )
                yield {"event": "done", "data": {"reason": "error", "detail": "empty stream from ollama"}}
                return

            content = "".join(content_parts)

            if not tool_calls:
                if not content.strip():
                    # Llama-3.2:1b sometimes returns zero tokens after a tool result.
                    # Surface a sentinel so the user doesn't see an empty bubble.
                    yield {"event": "token", "data": {"delta": "(no response from model)"}}
                logger.info(
                    "Agent stream finished user=%s hops=%d reason=complete tokens=%d",
                    self.user_id, hops_completed, len(content),
                )
                yield {"event": "done", "data": {"reason": "complete"}}
                return

            messages.append(
                {
                    "role": "assistant",
                    "content": content,
                    "tool_calls": _normalize_tool_calls(tool_calls),
                }
            )

            for call in tool_calls:
                fn = _get(_get(call, "function", {}), "name", "")
                raw_args = _get(_get(call, "function", {}), "arguments", {})
                args = _coerce_args(raw_args)

                # Emit `tool_call started` BEFORE awaiting the C-engine so the
                # frontend can show "Agent is querying the C-engine…" while
                # local inference is paused on slow CPU hardware.
                yield {
                    "event": "tool_call",
                    "data": {"name": fn, "arguments": args, "status": "started"},
                }

                impl = self._tools.get(fn)
                if impl is None:
                    # Use print() instead of logger so it shows up regardless of
                    # uvicorn's log config — this is intentionally high-visibility.
                    print(f"[TOOL CALL TRIGGERED] Tool: {fn} | Args: {args} | (unknown tool — no C wrapper invoked)", flush=True)
                    tool_result: dict = {"error": f"Unknown tool: {fn}"}
                    yield {
                        "event": "tool_call",
                        "data": {
                            "name": fn,
                            "arguments": args,
                            "status": "failed",
                            "message": "unknown tool",
                        },
                    }
                else:
                    print(f"[TOOL CALL TRIGGERED] Tool: {fn} | Args: {json.dumps(args, ensure_ascii=False)}", flush=True)
                    try:
                        tool_result = await impl(args)
                        print(
                            f"[C-ENGINE RAW RESPONSE] Tool: {fn} | Output: "
                            f"{json.dumps(tool_result, ensure_ascii=False, default=str)}",
                            flush=True,
                        )
                        yield {
                            "event": "tool_call",
                            "data": {
                                "name": fn,
                                "arguments": args,
                                "status": "completed",
                            },
                        }
                    except Exception as e:
                        logger.exception("Tool %s failed", fn)
                        tool_result = {"error": f"{type(e).__name__}: {e}"}
                        print(f"[C-ENGINE RAW RESPONSE] Tool: {fn} | Output: {tool_result}", flush=True)
                        yield {
                            "event": "tool_call",
                            "data": {
                                "name": fn,
                                "arguments": args,
                                "status": "failed",
                                "message": str(e),
                            },
                        }

                messages.append(
                    {
                        "role": "tool",
                        "name": fn,
                        "content": json.dumps(tool_result, ensure_ascii=False),
                    }
                )

        logger.info(
            "Agent stream finished user=%s hops=%d reason=tool_hop_limit",
            self.user_id, hops_completed,
        )
        yield {
            "event": "done",
            "data": {
                "reason": "tool_hop_limit",
                "detail": f"hit {self.max_tool_hops} hops",
            },
        }


# ---------------------------------------------------------------------------
# Helpers — the ollama python client returns either dicts or attr-style
# objects depending on version; these accessors keep us tolerant.
# ---------------------------------------------------------------------------
def _get(obj: Any, key: str, default: Any = None) -> Any:
    if obj is None:
        return default
    if isinstance(obj, dict):
        return obj.get(key, default)
    return getattr(obj, key, default)


def _coerce_args(raw: Any) -> dict:
    """Tool-call arguments arrive as a dict or a JSON string depending on
    the model. Normalize to a dict."""
    if raw is None:
        return {}
    if isinstance(raw, dict):
        return raw
    if isinstance(raw, str):
        try:
            parsed = json.loads(raw)
            return parsed if isinstance(parsed, dict) else {"_raw": parsed}
        except json.JSONDecodeError:
            return {"_raw": raw}
    return {}


def _normalize_tool_calls(tool_calls: list) -> list[dict]:
    out: list[dict] = []
    for tc in tool_calls:
        fn = _get(tc, "function", {})
        out.append(
            {
                "function": {
                    "name": _get(fn, "name", ""),
                    "arguments": _coerce_args(_get(fn, "arguments", {})),
                }
            }
        )
    return out
