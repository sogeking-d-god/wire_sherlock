import subprocess
import socket
import json
import os
import time
import uuid
import struct
import numpy as np
from enum import IntEnum

from api.python.c_ipc_manager import CEngineIPC, MetricType
from api.python.c_schemas import ParserResult
from api.config import ipc_config

# =====================================================================
# High-Level Wrapper
# =====================================================================
class WireSherlockSession:
    """
    The main clean interface for the Python Backend.
    Hides all IPC complexity and returns structured Pydantic models/Numpy arrays.
    """

    def __init__(self, pcap_path: str):
        self.ipc = CEngineIPC(pcap_path)
        self.analysis_summary: ParserResult = None
        self.metrics_cache = {}

    def start(self):
        """Starts the C engine."""
        print(f"[Python] Starting Session for {self.ipc.pcap_full_path}...")
        self.ipc.start_process_and_connect()

    def stop(self):
        """Stops the C engine gracefully."""
        self.ipc.close()
        print("[Python] Session closed.")

    def ping(self) -> bool:
        """Checks if the C engine is alive."""
        resp = self.ipc.send_command(ipc_config.CMD_PING)
        return resp is not None and resp.get('status') == 'status_success'

    def run_analysis(self) -> ParserResult:
        """
        Triggers the full PCAP parsing in C, receives the massive JSON,
        and parses it into a clean Pydantic model.
        """
        if not self.ipc.sock:
            print("[Python] Engine not started. Auto-starting...")
            self.start()

        response = self.ipc.send_command(ipc_config.CMD_START_ANALYSIS)

        if not response or response.get('status') != 'status_success':
            raise RuntimeError(f"Analysis failed: {response.get('data') if response else 'No response'}")

        # convert to dictionary
        self.analysis_summary = ParserResult(**response.get('data'))
        return self.analysis_summary

    def get_metric_bins(self, metric: MetricType) -> np.ndarray:
        """
        Requests a specific statistical metric from C.
        Returns a Numpy array directly.
        """
        if metric in self.metrics_cache:
            return self.metrics_cache[metric]

        response = self.ipc.send_command(ipc_config.CMD_GET_BINS, {"metric_id": int(metric)})

        if not response or response.get("status") != ipc_config.STATUS_BINARY:
            raise RuntimeError(f"Failed to fetch metric: {response.get('data')}")

        meta = response["data"]
        byte_size = int(meta["byte_size"])

        raw_binary = self.ipc.recv_binary(byte_size)
        if raw_binary is None:
            raise IOError("Incomplete binary stream received.")

        # create numpy array
        bins_array = np.frombuffer(raw_binary, dtype=np.float64).copy()
        self.metrics_cache[metric] = bins_array

        return bins_array


if __name__ == "__main__":
    session = WireSherlockSession("../pcap_files/regular_pcap_file.pcap")

    try:
        session.start()

        if session.ping():
            print("Engine is alive. Starting analysis...")

            result = session.run_analysis()

            print(f"Total Packets: {result.total_packets}")
            if result.global_ipv4_stats:
                print(f"Unique IPv4 Addresses: {len(result.global_ipv4_stats)}")

            traffic_bins = session.get_metric_bins(MetricType.BYTE_COUNT)
            print(f"Traffic Max Spikes: {np.max(traffic_bins)}")

    finally:
        session.stop()