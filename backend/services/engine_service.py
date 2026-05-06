import sys
import os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))
from api.python.session_wrapper import WireSherlockSession

_active_sessions = {}

def get_raw_analysis(pcap_path: str) -> dict:
    global _active_sessions

    pcap_path = os.path.abspath(pcap_path)

    if not os.path.exists(pcap_path):
        raise FileNotFoundError(f"PCAP file not found: {pcap_path}")

    if pcap_path in _active_sessions:
        session = _active_sessions[pcap_path]
    else:
        session = WireSherlockSession(pcap_path)
        session.start()
        _active_sessions[pcap_path] = session

    try:
        results = session.run_analysis()
        return results
    except Exception as e:
        print(f"[EngineService] Error running analysis: {e}")
        _active_sessions.pop(pcap_path, None)
        raise