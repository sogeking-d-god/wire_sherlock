import subprocess
import socket
import json
import os
import time
import uuid
import struct
import numpy as np

from config import ipc_config
from enum import IntEnum

class MetricType(IntEnum):
    PACKET_COUNT = 0
    BYTE_COUNT = 1
    SYN_COUNT = 2
    FIN_COUNT = 3
    RST_COUNT = 4

METRIC_NAMES = {
    MetricType.PACKET_COUNT: "Packet Count",
    MetricType.BYTE_COUNT: "Traffic Volume (Bytes)",
    MetricType.SYN_COUNT: "TCP SYN Flags",
    MetricType.FIN_COUNT: "TCP FIN Flags",
    MetricType.RST_COUNT: "TCP RST Flags"
}
class WireSherlockSession:
    def __init__(self, pcap_path):
        self.pcap_full_path = os.path.abspath(pcap_path)
        self.results_cache = {}
        self.parsing_summary = None
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

    def get_bins(self, metric_type: MetricType):
        if metric_type in self.results_cache:
            print(f"[Python] Returning {METRIC_NAMES[metric_type]} from Cache.")
            return self.results_cache[metric_type]

        payload = {"metric_id": int(metric_type)}
        response = self.send_command(ipc_config.CMD_GET_BINS, payload)

        if response and response.get("status") == ipc_config.STATUS_BINARY:
            meta = response["data"]
            byte_size = int(meta["byte_size"])

            raw_binary = self._recvall(byte_size)

            if raw_binary is None:
                print("[Python] Error: Failed to receive binary stream.")
                return None

            # convert the raw binary data to a numpy array of float64
            bins_array = np.frombuffer(raw_binary, dtype=np.float64).copy()

            self.results_cache[metric_type] = {
                "metadata": meta,
                "name": METRIC_NAMES.get(metric_type, "Unknown"),
                "data": bins_array
            }

            return self.results_cache[metric_type]

        else:
            error_msg = response.get("data") if response else "No response from C-Worker"
            print(f"[Python] Error fetching bins: {error_msg}")
            return None

if __name__ == "__main__":
    # יצירת סשן לקובץ ה-PCAP
    session = WireSherlockSession("../pcap_files/regular_pcap_file.pcap")

    try:
        session.start_engine()

        # 1. בדיקת קשר ראשונית
        if session.send_command(ipc_config.CMD_PING).get('status') == 'status_success':
            print("[Python] Connection to C-Engine: OK")

        # 2. הרצת הניתוח (Parsing)
        print("[Python] Running full packet analysis...")
        response = session.send_command(ipc_config.CMD_START_ANALYSIS)

        if response and response.get('status') == 'status_success':
            # שמירת סיכום הניתוח ב-RAM
            session.analysis_summary = response.get('data')
            print(f"[Python] Analysis Finished. Processed {session.analysis_summary.get('packet_count')} packets.")

            # 3. בקשת BINS עבור מטריקות ספציפיות
            # נבקש Byte Count ו-Packet Count
            for metric in [MetricType.BYTE_COUNT, MetricType.PACKET_COUNT]:
                print(f"[Python] Fetching {METRIC_NAMES[metric]}...")
                result = session.get_bins(metric)

                if result:
                    data_array = result['data']
                    print(f"   -> Success! Received {len(data_array)} bins.")
                    print(f"   -> Max Value in bins: {np.max(data_array)}")
                else:
                    print(f"   -> Failed to fetch {METRIC_NAMES[metric]}")

        else:
            print(f"[Python] Analysis failed: {response.get('data') if response else 'No response'}")

    except Exception as e:
        print(f"[Python] CRITICAL ERROR: {e}")
    finally:
        # סגירת המנוע והסוקט
        session.close()