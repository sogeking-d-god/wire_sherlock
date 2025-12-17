import dpkt
import os
from datetime import datetime

def process_pcap(filename, time_delta = 0.1):
    time_delta_arr = [0]

    if not os.path.exists(filename) or os.path.getsize(filename) == 0:
        raise FileNotFoundError(f"PCAP file not found or is empty: {filename}")

    pcap_reader = None

    with open(filename, 'rb') as f:


        try:
            pcap_reader = dpkt.pcap.Reader(f)
        except ValueError:
            f.seek(0)
            try:
                pcap_reader = dpkt.pcapng.Reader(f)
            except (AttributeError, ValueError) as e:
                raise Exception(f"Failed to read file as PCAP or PCAPNG. Check format or dpkt version. Error: {e}")

        if pcap_reader is None:
            raise Exception("PCAP Reader object could not be initialized.")

        first_timestamp = None
        for ts, _ in pcap_reader:
            if first_timestamp is None:
                first_timestamp = ts
            time_delta_arr.append(ts - first_timestamp)

    return time_delta_arr