// src/config.ts

export const API_BASE_URL = 'http://localhost:8000';

// All paths below are RELATIVE — pass them to apiFetch / apiJson, which
// prepends API_BASE_URL and always sends credentials.
export const API_ENDPOINTS = {
  AUTH: {
    LOGIN: '/auth/login',
    SIGNUP: '/auth/signup',
    LOGOUT: '/auth/logout',
    ME: '/auth/me',
  },

  CHAT: '/api/chat',

  // PCAP Management
  PCAP: {
    FILES: '/api/pcap/files',
    SESSION: '/api/pcap/active-session',
    UPLOAD: '/api/pcap/upload',
    SELECT: (fileId: string) => `/api/pcap/select/${fileId}`,
  },

  // Topology
  TOPOLOGY: {
    ANALYZE: '/api/topology/analyze',
  },

  // Metrics
  METRICS: {
    GET: (metricId: string) => `/api/metrics/${metricId}`,
  },
};
