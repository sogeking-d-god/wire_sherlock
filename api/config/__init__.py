import os

BASE_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
UPLOAD_DIR = os.path.join(BASE_DIR, "uploads")
DEMO_PATH = os.path.join(BASE_DIR, "pcap_files", "regular_pcap_file.pcap")

if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR)