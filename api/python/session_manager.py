import subprocess
import socket
import json
import os
import time
import uuid
import struct

from config import ipc_config

class WireSherlockSession:
    def __init__(self, pcap_path):
        self.pcap_full_path = os.path.abspath(pcap_path)
        # Creates a unique session ID and corresponding socket path for this session
        self.session_id = uuid.uuid4().hex[:8]
        self.socket_path = f"/tmp/wiresherlock_{self.session_id}.sock"
        self.c_process = None
        self.sock = None

    def start_engine(self):
        print(f"[Python] Starting C Engine for {self.pcap_full_path}...")
        print(f"[Python] Allocated Socket: {self.socket_path}")

        # Creating the subprocess for the C engine, passing the pcap path and socket path as arguments
        self.c_process = subprocess.Popen(
            ["./engine/core/c_worker", "--pcap", self.pcap_full_path, "--socket", self.socket_path],
            stdout=None,
            stderr=None
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

    def close(self):
        if hasattr(self, 'sock') and self.sock is not None:
            try:
                self.send_command(ipc_config.CMD_EXIT)
                self.sock.close()
                print("[Python] Session closed gracefully.")
            # If the C engine has already terminated or the socket is gone, just ignore any exceptions here
            except Exception:
                pass
        if hasattr(self, 'c_process') and self.c_process is not None:
            self.c_process.terminate()

    def _recvall(self, n):
        data = bytearray()
        while len(data) < n:
            packet = self.sock.recv(n - len(data))
            if not packet:
                return None
            data.extend(packet)
        return data

    def send_command(self, cmd_string, payload=None):
        if not self.sock: return None

        msg = {"cmd": cmd_string}
        if payload: msg.update(payload)

        msg_bytes = json.dumps(msg).encode('utf-8')

        # send the length of the message first (4 bytes, Network byte order)
        msg_length = len(msg_bytes)
        self.sock.sendall(struct.pack('!I', msg_length))

        self.sock.sendall(msg_bytes)

        # receive the length of the response first
        raw_msglen = self._recvall(4)
        if not raw_msglen: return None
        resp_len = struct.unpack('!I', raw_msglen)[0]

        resp_bytes = self._recvall(resp_len)
        return json.loads(resp_bytes.decode('utf-8'))

if __name__ == "__main__":
    session = WireSherlockSession("../pcap_files/regular_pcap_file.pcap")
    try:
        session.start_engine()

        response = session.send_command(ipc_config.CMD_PING)
        print(f"[Python] Ping Response: {response}")

        payload = {
            "bin_size": 100,
            "target_ips": ["192.168.1.1", "10.0.0.5"],
            "features": [1, 5, 8, 12]
        }
        response = session.send_command("fake_command_test", payload)
        print(f"[Python] Fake Command Response: {response}")

        response = session.send_command(ipc_config.CMD_START_ANALYSIS, None)

        if response is None:
            print("[Python] CRITICAL ERROR: C Engine crashed or closed the connection without responding!")
        elif response.get('status') == 'status_success':
            print("Full Analysis Results:")
            print(json.dumps(response['data'], indent=4))
        else:
            print(f"[Python] Analysis Failed! Reason: {response.get('data')}")
    finally:
        session.close()