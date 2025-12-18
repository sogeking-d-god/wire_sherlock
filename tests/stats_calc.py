import math
import numpy as np

"""!
@brief class for calculating EWMA and EWMVAR
"""
class Stat:
    def __init__(self, vals, window_size=0.1, ewma_alpha=0.125, ewmvar_alpha=0.125):

        if (len(vals) == 0):
            self.vals = np.array([])
            self.ewma_arr = self.ewmvar_arr = self.z_score_arr = np.array([])
            return

        self.vals = np.array(vals)
        self.ewma_alpha = ewma_alpha
        self.ewmvar_alpha = ewmvar_alpha

        self.ewma_arr = np.zeros(len(self.vals))
        self.ewmvar_arr = np.zeros(len(self.vals))
        self.z_score_arr = np.zeros(len(self.vals))

        self.ewma_arr[0] = self.vals[0]
        self.ewmvar_arr[0] = 0
        self.z_score_arr[0] = 0

        self._process_data()

    """!
    @brief function that calculates a single moving value based on old value, new value, and alpha
    """
    @staticmethod
    def calculate_ewm( old_ewm, new_val, alpha):
        return (1 - alpha) * old_ewm + alpha * new_val

    """!
    @brief function that fills ewma_arr, ewmvar_arr, and z_score_arr
    """
    def _process_data(self):

        for i in range(1,len(self.vals)):
            self.ewma_arr[i] = self.calculate_ewm(self.ewma_arr[i-1], self.vals[i], self.ewma_alpha)
            self.ewmvar_arr[i] = self.calculate_ewm(self.ewmvar_arr[i-1], (self.vals[i] - self.ewma_arr[i-1])**2, self.ewmvar_alpha)
            self.z_score_arr[i] = self.z_score(self.vals[i], self.ewma_arr[i-1], self.ewmvar_arr[i-1])


    @staticmethod
    def z_score(val,mean,var):
        if var == 0:
            return 0
        return abs((val - mean) / math.sqrt(var))