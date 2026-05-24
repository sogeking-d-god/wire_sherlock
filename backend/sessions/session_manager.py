import asyncio
import shutil
import uuid
from pathlib import Path
from typing import Optional

from fastapi import HTTPException, status

from api.python.session_wrapper import WireSherlockSession
from backend.sessions import workspace


class SessionManager:
    MAX_CONCURRENT = 8

    def __init__(self) -> None:
        self.sessions: dict[str, WireSherlockSession] = {}
        self.user_locks: dict[str, asyncio.Lock] = {}
        self.create_lock = asyncio.Lock()

    async def create_or_replace(
        self,
        user_id: str,
        src_pcap: str,
        label: str,
    ) -> WireSherlockSession:
        async with self.create_lock:
            if user_id in self.sessions:
                await self._terminate_locked(user_id)

            if len(self.sessions) >= self.MAX_CONCURRENT:
                raise HTTPException(
                    status_code=status.HTTP_503_SERVICE_UNAVAILABLE,
                    detail=f"Server at capacity ({self.MAX_CONCURRENT} active sessions)",
                )

            session_uuid = uuid.uuid4().hex
            workspace_dir = workspace.create(session_uuid)
            pcap_copy = workspace_dir / "pcap.bin"
            shutil.copyfile(src_pcap, pcap_copy)

            socket_path = workspace_dir / "ipc.sock"
            sess = WireSherlockSession(
                pcap_path=str(pcap_copy),
                socket_path=str(socket_path),
                workspace_dir=workspace_dir,
                session_uuid=session_uuid,
                label=label,
            )
            try:
                await sess.start_async()
                await sess.run_analysis_async()
            except Exception:
                await sess.aclose()
                raise

            self.sessions[user_id] = sess
            self.user_locks[user_id] = asyncio.Lock()
            return sess

    def get(self, user_id: str) -> Optional[WireSherlockSession]:
        return self.sessions.get(user_id)

    def get_label(self, user_id: str) -> Optional[str]:
        sess = self.sessions.get(user_id)
        return sess.label if sess else None

    def get_lock(self, user_id: str) -> asyncio.Lock:
        lock = self.user_locks.get(user_id)
        if lock is None:
            lock = asyncio.Lock()
            self.user_locks[user_id] = lock
        return lock

    async def terminate(self, user_id: str) -> None:
        async with self.create_lock:
            await self._terminate_locked(user_id)

    async def _terminate_locked(self, user_id: str) -> None:
        sess = self.sessions.pop(user_id, None)
        self.user_locks.pop(user_id, None)
        if sess is not None:
            await sess.aclose()

    async def terminate_all(self) -> None:
        async with self.create_lock:
            user_ids = list(self.sessions.keys())
            for uid in user_ids:
                await self._terminate_locked(uid)
        workspace.purge_all()

    def snapshot(self) -> list[tuple[str, WireSherlockSession]]:
        return list(self.sessions.items())


manager = SessionManager()
