import subprocess
import socket
import json
import os
import time
import uuid
import struct
import numpy as np
from enum import IntEnum

from python.c_schemas import ParserResult
import config.ipc_config as ipc_config
class MetricType(IntEnum):
    PACKET_COUNT = 0
    BYTE_COUNT = 1
    SYN_COUNT = 2
    FIN_COUNT = 3
    RST_COUNT = 4

# =====================================================================
# Low-Level IPC
# =====================================================================
class CEngineIPC:
    """Handles the low-level Unix Domain Socket communication with the C process."""

    def __init__(self, pcap_path: str):
        """Initializes the IPC handler with the given PCAP path. and prepares the socket path and session ID."""
        self.pcap_full_path = os.path.abspath(pcap_path)
        self.session_id = uuid.uuid4().hex[:8]
        self.socket_path = f"/tmp/wiresherlock_{self.session_id}.sock"
        self.c_process = None
        self.sock = None

    def start_process_and_connect(self):
        """Starts the C engine process and establishes the socket connection."""

        self.c_process = subprocess.Popen(
            ["./engine/core/c_worker", "--pcap", self.pcap_full_path, "--socket", self.socket_path],
            stdout=None, stderr=None
        )
        time.sleep(0.5) # Wait a moment for the C process to start and create the socket

        if not os.path.exists(self.socket_path):
            raise ConnectionError(f"C Engine failed to create socket at {self.socket_path}")

        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self.socket_path)

    def close(self):
        """Closes the socket and terminates the C process."""
        if self.sock:
            try:
                self.send_command(ipc_config.CMD_EXIT)
                self.sock.close()
            except Exception:
                pass
        if self.c_process:
            self.c_process.terminate()

    def _recvall(self, n: int) -> bytearray:
        """Helper function to receive exactly n bytes from the socket."""
        data = bytearray()
        while len(data) < n:
            packet = self.sock.recv(n - len(data))
            if not packet:
                return None
            data.extend(packet)
        return data

    def send_command(self, cmd_string: str, payload: dict = None) -> dict:
        """Sends a JSON command to C and returns the JSON response."""
        if not self.sock: raise ConnectionError("Socket not connected")

        msg = {"cmd": cmd_string}
        if payload: msg.update(payload)
        msg_bytes = json.dumps(msg).encode('utf-8')

        # Send Header (Length) + Payload
        self.sock.sendall(struct.pack('!I', len(msg_bytes))) # !I: means that the length is sent as an unsigned int in network byte order
        self.sock.sendall(msg_bytes)

        # Receive Header (Length) + Payload
        raw_msglen = self._recvall(4)
        if not raw_msglen: return None
        resp_len = struct.unpack('!I', raw_msglen)[0]

        resp_bytes = self._recvall(resp_len) # the payload
        return json.loads(resp_bytes.decode('utf-8'))

    def recv_binary(self, byte_size: int) -> bytes:
        """Receives a raw binary payload (used for numpy arrays)."""
        return self._recvall(byte_size)