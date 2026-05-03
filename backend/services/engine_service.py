import sys
import os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))
from api.python.session_wrapper import SessionWrapper

def get_raw_analysis(pcap_path: str) -> dict:
    """
    The function takes a path to a PCAP file, runs the analysis using the SessionWrapper, and returns the raw results as a dictionary.
    """
    if not os.path.exists(pcap_path):
        raise FileNotFoundError(f"PCAP file not found: {pcap_path}")

    session = SessionWrapper(pcap_path)
    try:
        #  START_ANALYSIS
        results = session.run_analysis()
        return results
    except Exception as e:
        print(f"[EngineService] Error running analysis: {e}")
        raise
    finally:

        pass