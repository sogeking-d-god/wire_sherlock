import json
import os
import socket
import struct
import subprocess
import time
from enum import IntEnum

import api.config.ipc_config as ipc_config


class MetricType(IntEnum):
    PACKET_COUNT = 0
    BYTE_COUNT = 1
    SYN_COUNT = 2
    FIN_COUNT = 3
    RST_COUNT = 4
    ACK_COUNT = 5
    PUSH_COUNT = 6


class SessionTerminatedError(RuntimeError):
    """The C worker died or its socket closed mid-request."""


# =====================================================================
# Low-Level IPC
# =====================================================================
class CEngineIPC:
    """Handles the low-level Unix Domain Socket communication with the C process.

    Socket path and workspace directory are injected by the caller so the
    SessionManager owns the per-session filesystem layout.
    """

    SOCKET_READY_TIMEOUT = 5.0
    SOCKET_READY_POLL = 0.05

    def __init__(self, pcap_path: str, socket_path: str, workspace_dir: str):
        self.pcap_full_path = os.path.abspath(pcap_path)
        self.socket_path = socket_path
        self.workspace_dir = workspace_dir
        self.c_process: subprocess.Popen | None = None
        self.sock: socket.socket | None = None

    def start_process_and_connect(self) -> None:
        """Starts the C engine process and establishes the socket connection."""
        self.c_process = subprocess.Popen(
            [
                "./api/engine/core/c_worker",
                "--pcap", self.pcap_full_path,
                "--socket", self.socket_path,
            ],
            stdout=None,
            stderr=None,
        )

        # Poll for socket creation instead of a fixed sleep.
        deadline = time.monotonic() + self.SOCKET_READY_TIMEOUT
        while time.monotonic() < deadline:
            if os.path.exists(self.socket_path):
                break
            if self.c_process.poll() is not None:
                raise ConnectionError(
                    f"C Engine exited before creating socket (rc={self.c_process.returncode})"
                )
            time.sleep(self.SOCKET_READY_POLL)
        else:
            raise ConnectionError(
                f"C Engine failed to create socket at {self.socket_path}"
            )

        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self.socket_path)

    def _recvall(self, n: int) -> bytearray | None:
        data = bytearray()
        while len(data) < n:
            packet = self.sock.recv(n - len(data))
            if not packet:
                return None
            data.extend(packet)
        return data

    def send_command(self, cmd_string: str, payload: dict | None = None) -> dict | None:
        """Sends a JSON command to C and returns the JSON response.

        Hardening Constraint #3: any low-level socket error from a dead
        C worker is converted to SessionTerminatedError so the async layer
        can surface it as a clean 409 instead of a 500.
        """
        if not self.sock:
            raise ConnectionError("Socket not connected")

        msg = {"cmd": cmd_string}
        if payload:
            msg.update(payload)
        msg_bytes = json.dumps(msg).encode("utf-8")

        try:
            self.sock.sendall(struct.pack("!I", len(msg_bytes)))
            self.sock.sendall(msg_bytes)

            raw_msglen = self._recvall(4)
            if not raw_msglen:
                raise SessionTerminatedError("C engine closed socket before response header")
            resp_len = struct.unpack("!I", raw_msglen)[0]

            resp_bytes = self._recvall(resp_len)
            if resp_bytes is None:
                raise SessionTerminatedError("C engine closed socket mid-response body")
            return json.loads(resp_bytes.decode("utf-8"))
        except (ConnectionResetError, BrokenPipeError, OSError) as e:
            raise SessionTerminatedError(f"C engine IPC failed: {e}") from e

    def recv_binary(self, byte_size: int) -> bytes:
        try:
            data = self._recvall(byte_size)
        except (ConnectionResetError, BrokenPipeError, OSError) as e:
            raise SessionTerminatedError(f"C engine binary IPC failed: {e}") from e
        if data is None:
            raise SessionTerminatedError("C engine closed socket during binary read")
        return bytes(data)

    def terminate(self, grace: float = 5.0) -> None:
        """Synchronous SIGTERM→SIGKILL escalation (Hardening Constraint #4).

        Sync because it must run to completion during shutdown / cleanup
        without yielding control. The async wrapper schedules it via
        loop.run_in_executor so it does not block the event loop.

        Workspace destruction is owned here and runs ONLY after the OS has
        confirmed the child is reaped — never before.
        """
        # 1. Graceful exit attempt (best effort, swallow everything).
        if self.sock is not None:
            try:
                self.sock.settimeout(1.0)
                self.send_command(ipc_config.CMD_EXIT)
            except Exception:
                pass

        # 2-4. SIGTERM, bounded wait, SIGKILL escalation.
        if self.c_process is not None:
            if self.c_process.poll() is None:
                try:
                    self.c_process.terminate()
                except Exception:
                    pass
                try:
                    self.c_process.wait(timeout=grace)
                except subprocess.TimeoutExpired:
                    try:
                        self.c_process.kill()
                    except Exception:
                        pass
                    try:
                        self.c_process.wait(timeout=grace)
                    except Exception:
                        pass

        # 5. Close socket FD.
        if self.sock is not None:
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None

        # 6. Workspace destroy — ONLY after child is reaped.
        from backend.sessions import workspace
        reaped = self.c_process is None or self.c_process.returncode is not None
        if reaped and self.workspace_dir:
            workspace.destroy_path(self.workspace_dir)

    # Backwards-compatible alias used by older tests / scripts.
    def close(self) -> None:
        self.terminate()
