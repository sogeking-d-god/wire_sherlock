import socket
import json
import os

class IPCManager:
    def __init__(self, socket_path="/tmp/wiresherlock.sock"):
        self.socket_path = socket_path

    def send_command(self, command, params=None):
        if not os.path.exists(self.socket_path):
            return {"status": "error", "message": "C Engine not running"}

        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
            client.connect(self.socket_path)
            req = {"command": command, "params": params or {}}
            client.sendall(json.dumps(req).encode())

            # קבלת התגובה (כאן צריך לוודא Buffer מספיק גדול ל-JSON הענק)
            data = b""
            while True:
                chunk = client.recv(4096)
                if not chunk: break
                data += chunk
            return json.loads(data.decode())