import os
import numpy as np
import matplotlib.pyplot as plt
import ruptures as rpt
from pcap_parser import process_pcap
from features import FeatureExtractor
from stats_calc import Stat

WINDOW_SIZES = [0.1]
PENALTY_VALUES = [10, 50, 100, 200, 500, 1000, 1500, 2000]

def pelt_function(arr, pen_val, model="l2"):
    if len(arr) < 2: return []
    np_arr = np.array(arr)
    algo = rpt.Pelt(model=model).fit(np_arr)
    return algo.predict(pen=pen_val)

def plot_tuning_result(data, cps, title, output_path, method_name, sig_type, win_size):
    plt.figure(figsize=(14, 6))
    plt.plot(data, label=f"{sig_type} Signal", color='blue', linewidth=1)

    for i, cp in enumerate(cps):
        label = "Change Point" if i == 0 else None
        plt.axvline(x=cp, color='red', linestyle='--', alpha=0.7, label=label)

    if method_name == "TimeDiff":
        plt.xlabel("Packet Index (#)")
        plt.ylabel("Inter-Arrival Time (ms)")
    else:
        plt.xlabel(f"Time Bins (Units of {win_size}s)")
        if sig_type == "ZScore":
            plt.ylabel("Z-Score (Standard Deviations)")
        else:
            plt.ylabel("Packet Count / Volume")

    plt.title(f"Method: {method_name} | Signal: {sig_type} | Win: {win_size}s\n{title}")
    plt.grid(True, which='both', linestyle='--', alpha=0.5)
    plt.legend()

    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()

def run_tuning(pcap_file):
    raw_timestamps = process_pcap(pcap_file)
    file_name_clean = os.path.basename(pcap_file).replace('.', '_')
    base_dir = f"tuning_results_{file_name_clean}"

    for win_size in WINDOW_SIZES:
        extractor = FeatureExtractor(raw_timestamps, window_size=win_size)

        datasets = {
            "FixedBins": (extractor.get_fixed_window_count(), "l1"),
            "SlidingWin": (extractor.get_sliding_window_count(), "l2"),
            "TimeDiff": (extractor.get_inter_arrival_times(), "l2"),
        }

        for name, (data, model) in datasets.items():
            if len(data) == 0: continue

            s = Stat(data)
            signals_to_test = {"Raw": s.vals, "EWMA": s.ewma_arr, "ZScore": s.z_score_arr}

            for sig_name, signal in signals_to_test.items():
                save_dir = os.path.join(base_dir, f"Win_{win_size}", f"{name}_{sig_name}")
                os.makedirs(save_dir, exist_ok=True)

                for pen in PENALTY_VALUES:
                    file_name = f"{name}_{sig_name}_WIN_{win_size}_PEN_{pen}.png"
                    output_path = os.path.join(save_dir, file_name)

                    if os.path.exists(output_path):
                        print(f"Skipping: {file_name} (Already exists)")
                        continue

                    cps = pelt_function(signal, pen_val=pen, model=model)

                    plot_title = f"Penalty: {pen} | Points Detected: {len(cps)}"
                    plot_tuning_result(signal, cps, plot_title, output_path, name, sig_name, win_size)

                    print(f"Processed: {name} | {sig_name} | Win: {win_size} | Pen: {pen}")

                    if len(cps) <= 1:
                        print(f"Stopped early for {sig_name} - Penalty too high.")
                        break

if __name__ == "__main__":
    pcap = "regular_pcap_file.pcap"
    if os.path.exists(pcap):
        run_tuning(pcap)
    else:
        print(f"File {pcap} not found.")