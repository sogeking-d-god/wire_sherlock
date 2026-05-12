import { useState, useEffect, useRef } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip } from 'recharts';
import { Activity, Download, TrendingUp, Settings, Maximize2, Loader2 } from 'lucide-react';

import { API_ENDPOINTS } from '../../config';
import { useFileContext } from '../context/FileContext';

// Available metrics to track
type MetricType = 'packets' | 'bytes' | 'synFlag' | 'rstFlag' | 'finFlag' | 'ackFlag' | 'pshFlag';

interface MetricConfig {
  key: MetricType;
  label: string;
  color: string;
  metricId: number;
}

const availableMetrics: MetricConfig[] = [
  { key: 'packets', label: 'Packets/sec', color: '#00a3ff', metricId: 0 },
  { key: 'bytes', label: 'Bytes/sec', color: '#10b981', metricId: 1 },
  { key: 'synFlag', label: 'SYN Flags', color: '#fb923c', metricId: 2 },
  { key: 'rstFlag', label: 'RST Flags', color: '#f43f5e', metricId: 3 },
  { key: 'finFlag', label: 'FIN Flags', color: '#a855f7', metricId: 4 },
  { key: 'ackFlag', label: 'ACK Flags', color: '#eab308', metricId: 5 },
  { key: 'pshFlag', label: 'PSH Flags', color: '#06b6d4', metricId: 6 },
];

// Backend API response structure
interface MetricResponse {
  metric_name: string;
  start_ts: number;
  bin_size_ms: number;
  values: number[];
  unit: string;
}

const BACKEND_URL = 'http://localhost:8000';

interface TrafficChartProps {
  onToggleView?: () => void;
  isFullView?: boolean;
}

export function TrafficChart({ onToggleView, isFullView = false }: TrafficChartProps) {
  const [selectedMetrics, setSelectedMetrics] = useState<MetricType[]>(['packets', 'bytes']);
  const chartContainerRef = useRef<HTMLDivElement>(null);
  const [chartDims, setChartDims] = useState({ width: 0, height: 0 });
  const [showMetricSelector, setShowMetricSelector] = useState(false);

  // State for fetched metrics
  const [metricsData, setMetricsData] = useState<Map<number, MetricResponse>>(new Map());
  const [isLoading, setIsLoading] = useState(true);
  const [isProcessing, setIsProcessing] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [chartData, setChartData] = useState<any[]>([]);

  const { activeFileId } = useFileContext();

  // Directly fetch metrics
  useEffect(() => {
    const fetchMetrics = async () => {
      // 1. Guard clause - don't fetch if no file is selected
      if (!activeFileId) return;

      setIsLoading(true);
      setIsProcessing(false);
      setError(null);

      try {
        const newMetricsData = new Map<number, MetricResponse>();

        // 2. Fetching in parallel using our centralized config
        const fetchPromises = availableMetrics.map(async (metric) => {
          // Use the dynamic helper from our config
          const url = API_ENDPOINTS.METRICS.GET(metric.metricId.toString());

          try {
            const resp = await fetch(url, {
              method: 'GET',
              headers: {
                'Accept': 'application/json',
              },
            });

            // Handle 202 - Processing
            if (resp.status === 202) {
              setIsProcessing(true);
              return null;
            }

            if (!resp.ok) return null;

            const data: MetricResponse = await resp.json();
            return { id: metric.metricId, data };
          } catch (err) {
            return null;
          }
        });

        const results = await Promise.all(fetchPromises);

        // 3. Handle retry logic if backend is still processing
        if (isProcessing) {
          setTimeout(() => fetchMetrics(), 2000);
          return;
        }

        // 4. Update states
        results.forEach(result => {
          if (result && result.data.values && result.data.values.length > 0) {
            newMetricsData.set(result.id, result.data);
          }
        });

        if (newMetricsData.size === 0) {
          throw new Error('No metrics data available for this file');
        }

        setMetricsData(newMetricsData);
        setChartData(reconstructChartData(newMetricsData));

      } catch (err) {
        const msg = err instanceof Error ? err.message : String(err);
        setError(`Failed to load metrics: ${msg}`);
      } finally {
        if (!isProcessing) setIsLoading(false);
      }
    };

    fetchMetrics();
  }, [activeFileId]); // Reload when the file changes

  // 1. Define a clear interface for what a chart data point looks like
  interface ChartDataPoint {
    time: string;
    timestamp: number;
    [key: string]: string | number; // This allows dynamic keys from availableMetrics
  }

  // Reconstruct chart data from backend metrics
  const reconstructChartData = (metrics: Map<number, MetricResponse>): ChartDataPoint[] => {
    let maxLength = 0;
    let baseMetric: MetricResponse | null = null;

    // Use the metrics Map values explicitly
    Array.from(metrics.values()).forEach((metric) => {
      if (metric.values.length > maxLength) {
        maxLength = metric.values.length;
        baseMetric = metric;
      }
    });

    // Type guard: ensure baseMetric is found and cast it
    if (!baseMetric || maxLength === 0) {
      console.warn('No metrics data available for chart reconstruction');
      return [];
    }

    // Tell TS explicitly that baseMetric is a MetricResponse here
    const safeBase: MetricResponse = baseMetric;

    console.log('Reconstructing chart data:', {
      baseMetric: safeBase.metric_name,
      dataPoints: maxLength,
      binSize: `${safeBase.bin_size_ms}ms`
    });

    const data: ChartDataPoint[] = [];

    for (let i = 0; i < maxLength; i++) {
      const timestamp = safeBase.start_ts + (i * safeBase.bin_size_ms / 1000);
      const date = new Date(timestamp * 1000);
      const timeStr = date.toLocaleTimeString('en-US', {
        hour12: false,
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
      });

      // Initialize with typed object
      const dataPoint: ChartDataPoint = {
        time: timeStr,
        timestamp: timestamp * 1000,
      };

      // Map each metric to its corresponding key
      availableMetrics.forEach(metricConfig => {
        const metricData = metrics.get(metricConfig.metricId);
        // Ensure we don't access out of bounds
        if (metricData && metricData.values && i < metricData.values.length) {
          dataPoint[metricConfig.key] = metricData.values[i];
        } else {
          dataPoint[metricConfig.key] = 0;
        }
      });

      data.push(dataPoint);
    }

    return data;
  };

  // Measure container with ResizeObserver — bypasses ResponsiveContainer's iframe measurement bug
  useEffect(() => {
    const el = chartContainerRef.current;
    if (!el) return;
    const ro = new ResizeObserver(entries => {
      const { width, height } = entries[0].contentRect;
      if (width > 0 && height > 0) setChartDims({ width: Math.floor(width), height: Math.floor(height) });
    });
    ro.observe(el);
    return () => ro.disconnect();
  }, []);

  const toggleMetric = (metric: MetricType) => {
    setSelectedMetrics(prev => {
      if (prev.includes(metric)) {
        // Don't allow removing all metrics
        if (prev.length === 1) return prev;
        return prev.filter(m => m !== metric);
      } else {
        return [...prev, metric];
      }
    });
  };

  return (
    <div className="relative h-full bg-[#0b1326] flex flex-col">
      {/* Header */}
      <div className="h-12 bg-[#060e20]/80 backdrop-blur border-b border-[#31394d] flex items-center justify-between px-4 shrink-0 relative z-20">
        <div className="flex items-center gap-2">
          <Activity className="w-4 h-4 text-[#00a3ff]" />
          <span className="font-bold text-[#dae2fd] text-[11px] tracking-[0.88px]">
            TRAFFIC_METRICS
          </span>
        </div>
        <div className="flex items-center gap-4">
          {/* Legend with data info - always visible */}
          <div className="flex items-center gap-3">
            {!isLoading && metricsData.size > 0 && (
              <div className="text-[8px] text-[#475569] mr-2">
                {metricsData.size}/{availableMetrics.length} metrics loaded
              </div>
            )}
            {selectedMetrics.map(metricKey => {
              const metric = availableMetrics.find(m => m.key === metricKey);
              if (!metric) return null;
              const hasData = metricsData.has(metric.metricId);
              return (
                <div key={metric.key} className="flex items-center gap-2">
                  <div className="w-3 h-3 rounded-sm" style={{ backgroundColor: hasData ? metric.color : '#31394d' }} />
                  <span className={`text-[9px] ${hasData ? 'text-[#88919d]' : 'text-[#475569]'}`}>
                    {metric.label}
                  </span>
                </div>
              );
            })}
          </div>

          {/* Metric Selector - always visible */}
          <button
            onClick={() => setShowMetricSelector(!showMetricSelector)}
            className={`w-7 h-7 flex items-center justify-center rounded hover:bg-[#171f33] ${
              showMetricSelector ? 'bg-[#171f33] text-[#00a3ff]' : 'text-[#88919d]'
            }`}
          >
            <Settings className="w-4 h-4" />
          </button>

          <button className="w-7 h-7 flex items-center justify-center rounded hover:bg-[#171f33] text-[#88919d]">
            <TrendingUp className="w-4 h-4" />
          </button>
          <button className="w-7 h-7 flex items-center justify-center rounded hover:bg-[#171f33] text-[#88919d]">
            <Download className="w-4 h-4" />
          </button>
          {onToggleView && (
            <button
              onClick={onToggleView}
              className="w-7 h-7 flex items-center justify-center rounded hover:bg-[#171f33] text-[#88919d]"
              title={isFullView ? "Return to Dashboard" : "Metrics Only"}
            >
              <Maximize2 className="w-4 h-4" />
            </button>
          )}
        </div>
      </div>

      {/* Metric Selector Panel - Absolute positioned overlay */}
      {showMetricSelector && (
        <div className="absolute top-12 left-0 right-0 bg-[#060e20]/95 backdrop-blur border-b border-[#31394d] px-4 py-3 z-10 shadow-lg">
          <div className="text-[#64748b] text-[10px] font-bold tracking-[0.8px] mb-2">
            SELECT METRICS FOR WORKING GRAPH
          </div>
          <div className="grid grid-cols-4 gap-2">
            {availableMetrics.map(metric => (
              <button
                key={metric.key}
                onClick={() => toggleMetric(metric.key)}
                className={`px-3 py-2 rounded text-[10px] font-bold border transition-colors ${
                  selectedMetrics.includes(metric.key)
                    ? 'border-current text-[#00a3ff] bg-[#00a3ff]/10'
                    : 'border-[#31394d] text-[#64748b] hover:border-[#64748b]'
                }`}
              >
                <div className="flex items-center gap-2">
                  <div className="w-2 h-2 rounded-full" style={{ backgroundColor: metric.color }} />
                  {metric.label}
                </div>
              </button>
            ))}
          </div>
        </div>
      )}

      {/* Single Working Graph */}
      <div ref={chartContainerRef} className="flex-1 min-h-0 p-4 relative">
        {/* Loading/Error/Empty State Overlay */}
        {(isLoading || error || chartData.length === 0) && (
          <div className="absolute inset-4 flex items-center justify-center z-10 pointer-events-none">
            <div className="bg-[#060e20]/90 backdrop-blur border border-[#31394d] rounded px-6 py-4 text-center">
              {isLoading ? (
                <>
                  <Loader2 className="w-5 h-5 text-[#00a3ff] animate-spin mx-auto mb-2" />
                  <div className="text-[#64748b] text-[11px] font-bold tracking-[0.88px] mb-1">
                    {isProcessing ? 'PROCESSING PCAP...' : 'LOADING METRICS'}
                  </div>
                  <div className="text-[#475569] text-[9px]">
                    {isProcessing ? 'Backend is still processing the PCAP file. This may take a few moments...' : 'Fetching traffic metrics data...'}
                  </div>
                </>
              ) : error ? (
                <>
                  <div className="text-red-500 text-[11px] font-bold tracking-[0.88px] mb-1">
                    METRICS FETCH ERROR
                  </div>
                  <div className="text-[#475569] text-[9px]">
                    {error}
                  </div>
                </>
              ) : (
                <>
                  <div className="text-[#64748b] text-[11px] font-bold tracking-[0.88px] mb-1">
                    WAITING FOR ANALYSIS
                  </div>
                  <div className="text-[#475569] text-[9px]">
                    No metric data available yet
                  </div>
                </>
              )}
            </div>
          </div>
        )}

        {chartDims.width > 0 && chartDims.height > 0 && chartData.length > 0 && !isLoading && (
          <LineChart
            width={chartDims.width}
            height={chartDims.height}
            data={chartData}
            margin={{ top: 5, right: 20, left: 10, bottom: 5 }}
          >
            <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
            <XAxis
              dataKey="time"
              stroke="#64748b"
              tick={{ fill: '#64748b', fontSize: 10 }}
              tickLine={{ stroke: '#31394d' }}
              interval="preserveStartEnd"
            />
            <YAxis
              stroke="#64748b"
              tick={{ fill: '#64748b', fontSize: 10 }}
              tickLine={{ stroke: '#31394d' }}
            />
            <Tooltip
              contentStyle={{ backgroundColor: '#060e20', border: '1px solid #31394d', borderRadius: '4px', fontSize: '11px' }}
              labelStyle={{ color: '#dae2fd' }}
            />
            {/* All lines always mounted with stable keys — toggled via hide prop to avoid recharts key collisions */}
            {availableMetrics.map(metric => (
              <Line
                key={metric.key}
                hide={!selectedMetrics.includes(metric.key)}
                type="monotone"
                dataKey={metric.key}
                stroke={metric.color}
                strokeWidth={2}
                dot={false}
                name={metric.label}
                isAnimationActive={false}
              />
            ))}
          </LineChart>
        )}
      </div>
    </div>
  );
}