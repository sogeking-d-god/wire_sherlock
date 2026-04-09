import subprocess
import socket
import json
import os
import time
import uuid

import ipc_config

class WireSherlockSession:
    def __init__(self, pcap_path):
        self.pcap_path = pcap_path
        # Creates a unique session ID and corresponding socket path for this session
        self.session_id = uuid.uuid4().hex[:8]
        self.socket_path = f"/tmp/wiresherlock_{self.session_id}.sock"
        self.c_process = None
        self.sock = None

    def start_engine(self):
        print(f"[Python] Starting C Engine for {self.pcap_path}...")
        print(f"[Python] Allocated Socket: {self.socket_path}")

        # Creating the subprocess for the C engine, passing the pcap path and socket path as arguments
        self.c_process = subprocess.Popen(
            ["./c_worker", "--pcap", self.pcap_path, "--socket", self.socket_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )

        # Wait a moment for the C engine to initialize and create the socket
        time.sleep(0.5)
        self._connect_to_socket()

    def _connect_to_socket(self):
        if not os.path.exists(self.socket_path):
            raise Exception(f"C Engine failed to create socket at {self.socket_path}")

        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self.socket_path)
        print("[Python] Successfully connected to C Engine UDS!")

    def send_command(self, cmd_string, payload=None):
        if not self.sock:
            return None

        msg = {"cmd": cmd_string}
        if payload:
            msg.update(payload)

        self.sock.sendall(json.dumps(msg).encode('utf-8'))

        response_bytes = self.sock.recv(ipc_config.BUFFER_SIZE)
        return json.loads(response_bytes.decode('utf-8'))

    def close(self):
        print("[Python] Sending EXIT command to C Engine...")
        self.send_command(ipc_config.CMD_EXIT)
        self.sock.close()
        self.c_process.wait()
        print("[Python] Session closed gracefully.")

if __name__ == "__main__":
    # Simple test
    session = WireSherlockSession("test_capture.pcap")
    try:
        session.start_engine()

        response = session.send_command(ipc_config.CMD_PING)
        print(f"[Python] Response from C: {response}")

    finally:
        session.close()