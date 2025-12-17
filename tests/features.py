class FeatureExtractor:
    def __init__(self, timestamps, window_size=0.1):
        self.timestamps = np.array(timestamps)
        self.window_size = window_size

    def get_fixed_window_count(self):
        if len(self.timestamps) == 0:
            return []
        bins = np.zeros(self.timestamps[-1] // self.window_size + 1, dtype=int )
        for t in self.timestamps:
            bins[t // self.window_size] += 1
        return bins

    def get_sliding_window_count(self):
        countimestamps = []
        left_index = 0
        for i, current_timestamp in enumerate(self.timestamps):
            while self.timestamps[left_index] < current_timestamp - self.window_size:
                left_index += 1
            countimestamps.append(i - left_index + 1)
        return np.array(countimestamps)

    def get_inter_arrival_times(self):
        return np.diff(self.timestamps)