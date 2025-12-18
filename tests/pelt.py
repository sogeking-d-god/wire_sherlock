from stats_calc import Stat
from pcap_parser import process_pcap
import numpy as np
import ruptures as rpt
import matplotlib.pyplot as plt
from sys import argv as args
import os
from features import FeatureExtractor

"""!
@brief function that calculates change points with PELT algotrithm with different penalty and model
"""
def pelt_function(arr, pen_val=100, model="l2"):

    if len(arr) < 2:
        return []
    np_arr = np.array(arr)

    algo = rpt.Pelt(model=model).fit(np_arr)
    change_points_indices = algo.predict(pen=pen_val)
    return change_points_indices

def plot_analysis(data_arr, change_points, title = "",dir_name="."):

    time_series = np.arange(len(data_arr))

    plt.figure(figsize=(12, 6))
    plt.plot(time_series, data_arr, label=title)

    for cp in change_points:
        plt.axvline(x=cp, color='r', linestyle='--', linewidth=1, label='Change Point' if cp == change_points[0] else "")

    plt.title(f"Change Point Detection on {title}")
    plt.xlabel("Time Bins (100ms)")
    plt.ylabel(title)
    plt.legend(loc="best")
    output_filename = f"pelt_analysis_result_{title}.png"
    full_path = dir_name + "/" + output_filename
    plt.savefig(full_path)
    print(f"\nSUCCESS: Analysis complete. Result saved to {full_path}")
    plt.close()



def analyze_data(data, data_name, parent_dir, model="l2"):
    view_dir = os.path.join(parent_dir, data_name)
    os.makedirs(view_dir, exist_ok=True)

    stats = Stat(data)

    cp_vals = pelt_function(stats.vals, model=model)
    plot_analysis(stats.vals, cp_vals, f"{data_name}_vals", dir_name=view_dir)

    cp_ewma = pelt_function(stats.ewma_arr, model=model)
    plot_analysis(stats.ewma_arr, cp_ewma, f"{data_name}_EWMA", dir_name=view_dir)

    cp_z = pelt_function(stats.z_score_arr, model="l2")
    plot_analysis(stats.z_score_arr, cp_z, f"{data_name}_ZScore", dir_name=view_dir)

def main():
    if len(args) < 2:
        pcap_file = input("Enter the path to the pcap file: ")
    else:
        pcap_file = args[1]

    if not os.path.exists(pcap_file):
        print(f"Error: File {pcap_file} not found.")
        return

    timestamps_arr = process_pcap(pcap_file)

    extractor = FeatureExtractor(timestamps_arr, window_size=0.1)

    views = {
        "Fixed_Bins": (extractor.get_fixed_window_count(), "l1"),
        "Sliding_Window": (extractor.get_sliding_window_count(), "l2"),
        "Inter_Arrival_Times": (extractor.get_inter_arrival_times(), "l2")
    }

    results_base = f"results_{os.path.basename(pcap_file).replace('.', '_')}"


    for name, (data, model) in views.items():
        print(f"Running full analysis for: {name}...")
        analyze_data(data, name, results_base, model=model)

if __name__ == "__main__":
    main()