"""Tests for backend.sessions.session_manager.

Covers all eight cases from the verification plan in
~/.claude/plans/i-need-to-refactor-curried-eich.md.

WireSherlockSession is patched out so no real c_worker subprocess
is spawned. The patch installs a MockSession whose start_async /
run_analysis_async / aclose are awaitable no-ops.
"""

import asyncio
import time
from pathlib import Path
from unittest.mock import AsyncMock, MagicMock, patch

import pytest
from fastapi import HTTPException

from backend.sessions import reaper
from backend.sessions.session_manager import SessionManager


def make_mock_session(*, fail_start: bool = False, label: str = "label") -> MagicMock:
    sess = MagicMock()
    sess.start_async = AsyncMock(side_effect=RuntimeError("boom")) if fail_start else AsyncMock()
    sess.run_analysis_async = AsyncMock()
    sess.aclose = AsyncMock()
    sess.label = label
    sess.last_active = time.monotonic()
    sess.touch = lambda: setattr(sess, "last_active", time.monotonic())
    return sess


@pytest.fixture
def src_pcap(tmp_path: Path) -> str:
    src = tmp_path / "src.pcap"
    src.write_bytes(b"\xd4\xc3\xb2\xa1stubpcapdata")
    return str(src)


@pytest.fixture
def patched_workspace(tmp_path: Path):
    """Redirect the workspace root into tmp_path so tests can't pollute /tmp."""
    from backend.sessions import workspace as ws_mod

    sandbox = tmp_path / "wiresherlock"
    original = ws_mod.WORKSPACE_ROOT
    ws_mod.WORKSPACE_ROOT = sandbox
    yield sandbox
    ws_mod.WORKSPACE_ROOT = original


@pytest.fixture
def patched_session():
    """Swap WireSherlockSession for a MockSession factory inside session_manager."""
    created: list[MagicMock] = []

    def _factory(**kwargs):
        sess = make_mock_session(label=kwargs.get("label", "label"))
        created.append(sess)
        return sess

    with patch(
        "backend.sessions.session_manager.WireSherlockSession",
        side_effect=_factory,
    ) as p:
        p.created = created
        yield p


@pytest.mark.asyncio
async def test_per_user_isolation(src_pcap, patched_workspace, patched_session):
    mgr = SessionManager()
    s1 = await mgr.create_or_replace("u1", src_pcap, "a.pcap")
    s2 = await mgr.create_or_replace("u2", src_pcap, "b.pcap")

    assert mgr.get("u1") is s1
    assert mgr.get("u2") is s2
    assert s1 is not s2
    s1.aclose.assert_not_called()
    s2.aclose.assert_not_called()


@pytest.mark.asyncio
async def test_replace_terminates_old(src_pcap, patched_workspace, patched_session):
    mgr = SessionManager()
    s1 = await mgr.create_or_replace("u1", src_pcap, "first.pcap")
    s2 = await mgr.create_or_replace("u1", src_pcap, "second.pcap")

    s1.aclose.assert_awaited_once()
    assert mgr.get("u1") is s2
    assert s2.label == "second.pcap"


@pytest.mark.asyncio
async def test_idle_reaper(src_pcap, patched_workspace, patched_session):
    mgr = SessionManager()
    s = await mgr.create_or_replace("u1", src_pcap, "x.pcap")
    s.last_active = time.monotonic() - 5.0  # force "idle"

    task = asyncio.create_task(reaper.run(mgr, idle_seconds=0.1, poll_seconds=0.05))
    await asyncio.sleep(0.2)
    task.cancel()
    try:
        await task
    except asyncio.CancelledError:
        pass

    assert mgr.get("u1") is None
    s.aclose.assert_awaited()


@pytest.mark.asyncio
async def test_cap_rejects(src_pcap, patched_workspace, patched_session):
    mgr = SessionManager()
    mgr.MAX_CONCURRENT = 2
    await mgr.create_or_replace("u1", src_pcap, "a")
    await mgr.create_or_replace("u2", src_pcap, "b")

    with pytest.raises(HTTPException) as exc_info:
        await mgr.create_or_replace("u3", src_pcap, "c")
    assert exc_info.value.status_code == 503

    assert mgr.get("u1") is not None
    assert mgr.get("u2") is not None


@pytest.mark.asyncio
async def test_logout_cleanup(src_pcap, patched_workspace, patched_session):
    mgr = SessionManager()
    s = await mgr.create_or_replace("u1", src_pcap, "x")
    await mgr.terminate("u1")

    assert mgr.get("u1") is None
    s.aclose.assert_awaited_once()


@pytest.mark.asyncio
async def test_per_user_lock_serializes(src_pcap, patched_workspace, patched_session):
    """Two concurrent IPC calls for the same user must serialize.
    Different users must run in parallel (max concurrency 2).
    """
    mgr = SessionManager()
    s1 = await mgr.create_or_replace("u1", src_pcap, "a")
    s2 = await mgr.create_or_replace("u2", src_pcap, "b")

    lock_u1 = mgr.get_lock("u1")
    lock_u2 = mgr.get_lock("u2")

    in_flight_same = 0
    peak_same = 0
    in_flight_cross = 0
    peak_cross = 0

    async def call_same(_):
        nonlocal in_flight_same, peak_same
        async with lock_u1:
            in_flight_same += 1
            peak_same = max(peak_same, in_flight_same)
            await asyncio.sleep(0.02)
            in_flight_same -= 1

    async def call_cross(lock):
        nonlocal in_flight_cross, peak_cross
        async with lock:
            in_flight_cross += 1
            peak_cross = max(peak_cross, in_flight_cross)
            await asyncio.sleep(0.02)
            in_flight_cross -= 1

    await asyncio.gather(*(call_same(i) for i in range(4)))
    assert peak_same == 1

    await asyncio.gather(call_cross(lock_u1), call_cross(lock_u2))
    assert peak_cross == 2

    _ = s1, s2  # silence unused-warning


@pytest.mark.asyncio
async def test_shutdown_kills_all(src_pcap, patched_workspace, patched_session):
    mgr = SessionManager()
    sessions = []
    for uid in ("u1", "u2", "u3"):
        sessions.append(await mgr.create_or_replace(uid, src_pcap, uid))

    await mgr.terminate_all()

    for s in sessions:
        s.aclose.assert_awaited()
    assert mgr.get("u1") is None
    assert mgr.get("u2") is None
    assert mgr.get("u3") is None
    # Workspace root purged.
    from backend.sessions import workspace as ws_mod
    assert not ws_mod.WORKSPACE_ROOT.exists()


@pytest.mark.asyncio
async def test_sigterm_to_sigkill_escalation():
    """CEngineIPC.terminate escalates from SIGTERM to SIGKILL when wait() times out."""
    import subprocess as sp

    from api.python.c_ipc_manager import CEngineIPC

    mock_proc = MagicMock()
    mock_proc.poll.return_value = None  # alive
    mock_proc.returncode = None

    call_log = []

    def wait_side_effect(timeout=None):
        call_log.append(("wait", timeout))
        if mock_proc.kill.called:
            mock_proc.returncode = -9
            return -9
        raise sp.TimeoutExpired(cmd="c_worker", timeout=timeout)

    mock_proc.wait.side_effect = wait_side_effect
    mock_proc.terminate.side_effect = lambda: call_log.append(("terminate",))
    mock_proc.kill.side_effect = lambda: call_log.append(("kill",))

    ipc = CEngineIPC.__new__(CEngineIPC)
    ipc.pcap_full_path = "x"
    ipc.socket_path = "x"
    ipc.workspace_dir = ""
    ipc.c_process = mock_proc
    ipc.sock = None

    ipc.terminate(grace=0.05)

    kinds = [c[0] for c in call_log]
    assert "terminate" in kinds
    assert "kill" in kinds
    assert kinds.index("terminate") < kinds.index("kill")
