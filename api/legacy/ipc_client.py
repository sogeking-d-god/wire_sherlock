import socket
import json
import os

SOCKET_PATH = "/tmp/wiresherlock.sock"

def connect_via_uds():
    if not os.path.exists(SOCKET_PATH):
        print(f"[Python] Error: Socket file {SOCKET_PATH} not found.")
        return

    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
            print(f"[Python] Connecting to {SOCKET_PATH}...")
            s.connect(SOCKET_PATH)

            request_data = json.dumps({"command": "ping"})
            s.sendall(request_data.encode('utf-8'))

            data = s.recv(1024)
            response = json.loads(data.decode('utf-8'))

            print("\n[Python] Received Data:")
            # print(json.dumps(response, indent=4))

    except Exception as e:
        print(f"[Python] Connection error: {e}")

if __name__ == "__main__":
    connect_via_uds()