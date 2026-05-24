import asyncio
import time

from backend.sessions.session_manager import SessionManager

IDLE_TIMEOUT_SECONDS = 30 * 60
POLL_INTERVAL_SECONDS = 60


async def run(
    mgr: SessionManager,
    idle_seconds: float = IDLE_TIMEOUT_SECONDS,
    poll_seconds: float = POLL_INTERVAL_SECONDS,
) -> None:
    while True:
        try:
            await asyncio.sleep(poll_seconds)
            now = time.monotonic()
            for user_id, sess in mgr.snapshot():
                if now - sess.last_active > idle_seconds:
                    print(f"[Reaper] Idle session for user {user_id} — terminating")
                    try:
                        await mgr.terminate(user_id)
                    except Exception as e:
                        print(f"[Reaper] Error terminating user {user_id}: {e}")
        except asyncio.CancelledError:
            raise
        except Exception as e:
            print(f"[Reaper] Loop error: {e}")
