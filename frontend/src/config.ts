// src/config.ts

export const API_BASE_URL = 'http://localhost:8000';
export const API_ENDPOINTS = {
  // PCAP Management
  PCAP: {
    FILES: `${API_BASE_URL}/api/pcap/files`,
    SESSION: `${API_BASE_URL}/api/pcap/active-session`,
    UPLOAD: `${API_BASE_URL}/api/pcap/upload`,
    SELECT: (fileId: string) => `${API_BASE_URL}/api/pcap/select/${fileId}`,
  },

  // Topology
  TOPOLOGY: {
    ANALYZE: `${API_BASE_URL}/api/topology/analyze`,
  },

  // Metrics
  METRICS: {
    GET: (metricId: string) => `${API_BASE_URL}/api/metrics/${metricId}`,
  },
};