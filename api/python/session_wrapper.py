import asyncio
import time
from pathlib import Path
from typing import Any

import numpy as np
from fastapi import HTTPException, status

from api.config import ipc_config
from api.python.c_ipc_manager import CEngineIPC, MetricType, SessionTerminatedError
from api.python.c_schemas import (
    AnomalyBundle,
    AnomalyClusterResult,
    HttpAttackReport,
    MetricResult,
    ParserResult,
)


def _to_session_terminated(exc: SessionTerminatedError) -> HTTPException:
    return HTTPException(
        status_code=status.HTTP_409_CONFLICT,
        detail=f"Session was terminated mid-request: {exc}",
    )


class WireSherlockSession:
    """Per-user investigation session. Owns one C worker subprocess via CEngineIPC.

    The per-session asyncio.Lock serializes IPC calls — the underlying socket
    is a single full-duplex stream, so concurrent send/recv would interleave
    framed messages. Different users hold different sessions and therefore
    different locks, so cross-user concurrency is preserved.
    """

    def __init__(
        self,
        pcap_path: str,
        socket_path: str,
        workspace_dir: Path | str,
        session_uuid: str,
        label: str,
    ):
        self.pcap_path = pcap_path
        self.label = label
        self.session_uuid = session_uuid
        self.workspace_dir = Path(workspace_dir)
        self.ipc = CEngineIPC(
            pcap_path=pcap_path,
            socket_path=socket_path,
            workspace_dir=str(workspace_dir),
        )
        self.analysis_summary: ParserResult | None = None
        self.lock = asyncio.Lock()
        self.last_active: float = time.monotonic()

    def touch(self) -> None:
        self.last_active = time.monotonic()

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------
    async def start_async(self) -> None:
        loop = asyncio.get_running_loop()
        await loop.run_in_executor(None, self.ipc.start_process_and_connect)
        self.touch()

    async def aclose(self) -> None:
        loop = asyncio.get_running_loop()
        await loop.run_in_executor(None, self.ipc.terminate)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------
    async def _send(self, cmd: str, payload: dict | None = None) -> dict:
        loop = asyncio.get_running_loop()
        async with self.lock:
            try:
                resp = await loop.run_in_executor(
                    None, self.ipc.send_command, cmd, payload
                )
            except SessionTerminatedError as e:
                raise _to_session_terminated(e) from e
        self.touch()
        if resp is None:
            raise HTTPException(status_code=502, detail=f"Empty response from C engine for {cmd}")
        return resp

    @staticmethod
    def _require_success(resp: dict, cmd: str) -> Any:
        status_str = resp.get("status")
        if status_str != ipc_config.STATUS_SUCCESS:
            data = resp.get("data")
            raise HTTPException(
                status_code=502,
                detail=f"C engine error on {cmd}: {data}",
            )
        return resp.get("data")

    # ------------------------------------------------------------------
    # Existing commands
    # ------------------------------------------------------------------
    async def run_analysis_async(self) -> ParserResult:
        resp = await self._send(ipc_config.CMD_START_ANALYSIS)
        data = self._require_success(resp, ipc_config.CMD_START_ANALYSIS)
        self.analysis_summary = ParserResult(**data)
        return self.analysis_summary

    async def get_metric_bins_async(self, metric: MetricType) -> MetricResult:
        if not isinstance(metric, MetricType):
            metric = MetricType(metric)

        loop = asyncio.get_running_loop()
        async with self.lock:
            try:
                resp = await loop.run_in_executor(
                    None,
                    self.ipc.send_command,
                    ipc_config.CMD_GET_BINS,
                    {"metric_id": int(metric)},
                )
                if resp is None or resp.get("status") != ipc_config.STATUS_BINARY:
                    raise HTTPException(
                        status_code=502,
                        detail=f"C Engine failed to provide metric {metric.name}",
                    )
                meta = resp["data"]
                byte_size = int(meta["byte_size"])
                raw_binary = await loop.run_in_executor(
                    None, self.ipc.recv_binary, byte_size
                )
            except SessionTerminatedError as e:
                raise _to_session_terminated(e) from e
        self.touch()

        bins_array = np.frombuffer(raw_binary, dtype=np.float64).copy()
        return MetricResult(
            metric_id=meta["metric_id"],
            start_ts=meta["start_ts"],
            bin_size_ms=meta["bin_size_ms"],
            total_bins=meta["total_bins"],
            data=bins_array.tolist(),
        )

    async def ping_async(self) -> bool:
        resp = await self._send(ipc_config.CMD_PING)
        return resp.get("status") == ipc_config.STATUS_SUCCESS

    # ------------------------------------------------------------------
    # New advanced commands
    # ------------------------------------------------------------------
    async def analyze_http_advanced(self) -> HttpAttackReport:
        resp = await self._send(ipc_config.CMD_ANALYZE_HTTP)
        data = self._require_success(resp, ipc_config.CMD_ANALYZE_HTTP)
        return HttpAttackReport(**data)

    async def generate_anomalies(
        self,
        metric_mask: int,
        z_sensitivity: float | None = None,
    ) -> AnomalyBundle:
        payload: dict = {"metric_mask": int(metric_mask)}
        if z_sensitivity is not None:
            payload["z_sensitivity"] = float(z_sensitivity)
        resp = await self._send(ipc_config.CMD_GENERATE_ANOMALIES, payload)
        data = self._require_success(resp, ipc_config.CMD_GENERATE_ANOMALIES)
        return AnomalyBundle(**data)

    async def cluster_anomalies(
        self,
        metric_mask: int,
        macro_ids: list[int],
        micro_ids: list[int],
        cfg: dict | None = None,
    ) -> AnomalyClusterResult:
        payload: dict = {
            "metric_mask": int(metric_mask),
            "macro_ids": [int(i) for i in macro_ids],
            "micro_ids": [int(i) for i in micro_ids],
        }
        if cfg is not None:
            payload["cfg"] = cfg
        resp = await self._send(ipc_config.CMD_CLUSTER_ANOMALIES, payload)
        data = self._require_success(resp, ipc_config.CMD_CLUSTER_ANOMALIES)
        return AnomalyClusterResult(**data)
