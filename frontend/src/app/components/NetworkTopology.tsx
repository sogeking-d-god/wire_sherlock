import { useEffect, useRef, useState, useMemo } from 'react';
import { Network, DataSet } from 'vis-network/standalone';
import { Server, Maximize2, Search, ZoomIn, ZoomOut, Locate, X, ChevronDown, Loader2 } from 'lucide-react';
import { Collapsible, CollapsibleContent, CollapsibleTrigger } from './ui/collapsible';
import './NetworkTopology.css';

import { apiFetch } from '../../api/client';
import { API_ENDPOINTS } from '../../config';
import { useFileContext } from '../context/FileContext';


const PROTOCOL_MAP: { [key: number]: string } = {
  6: 'TCP',
  17: 'UDP',
  1: 'ICMP',
  2: 'IGMP',
  58: 'ICMPv6',
  89: 'OSPF',
};

// Internal types
interface SessionData {
  sessionId: string;
  protocol: string;
  sourceIp: string;
  sourcePort: number;
  destIp: string;
  destPort: number;
  startTime: number; // float Unix seconds
  endTime: number;   // float Unix seconds
  packets: number;
  bytes: number;
  startStatus: string;
  endStatus: string;
  hasReset: boolean;
  hasTimeout: boolean;
}

interface MacEntry {
  mac: string;
  packets_at_node: number;
  bytes_at_node: number;
}

interface GlobalMacData {
  mac: string;
  total_packets: number;
  total_bytes: number;
  associated_ips: string[];
}

interface NodeDetail {
  ip: string;
  macs: MacEntry[];
  packets: number;
  bytes: number;
}

// Backend API types — matches Pydantic schema exactly
interface BackendNodeData {
  id: string;
  label: string;
  ip: string;
  packets: number;
  bytes: number;
  mac: string;          // legacy single-MAC field
  macs?: MacEntry[];    // new multi-MAC array
}

interface BackendLinkData {
  id: string;
  src_ip: string;
  dst_ip: string;
  src_port: number;
  dst_port: number;
  start_time: number;   // float Unix seconds
  end_time: number;     // float Unix seconds
  protocol: number;
  bytes: number;
  packets: number;
  start_status: string; // e.g. "Handshake Complete"
  end_status: string;   // e.g. "Not Closed", "Closed (Reset)"
}

interface BackendResponse {
  nodes: BackendNodeData[];
  links: BackendLinkData[];
  macs: GlobalMacData[];
}

const BACKEND_URL = 'http://localhost:8000';

// Helpers
const formatBytes = (bytes: number): string => {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1048576) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / 1048576).toFixed(1)} MB`;
};

// Display a float Unix second timestamp as HH:MM:SS.mmm
const formatTimestampS = (unixS: number): string => {
  const date = new Date(unixS * 1000);
  const hms = date.toLocaleTimeString('en-US', { hour12: false });
  const ms = String(Math.floor((unixS % 1) * 1000)).padStart(3, '0');
  return `${hms}.${ms}`;
};

const getStatusStyle = (status: string): string => {
  const s = status.toLowerCase();
  if (s.includes('reset'))              return 'bg-red-500/20 text-red-400';
  if (s.includes('timeout'))            return 'bg-orange-500/20 text-orange-400';
  if (s.includes('handshake'))          return 'bg-emerald-500/20 text-emerald-400';
  if (s.includes('closed') && !s.includes('reset') && !s.includes('timeout'))
                                        return 'bg-emerald-500/20 text-emerald-400';
  if (s.includes('not closed'))         return 'bg-[#00a3ff]/20 text-[#00a3ff]';
  if (s.includes('established'))        return 'bg-[#00a3ff]/20 text-[#00a3ff]';
  if (s.includes('syn'))                return 'bg-amber-500/20 text-amber-400';
  return 'bg-[#64748b]/20 text-[#64748b]';
};

const parseBackendData = (raw: BackendResponse): { nodeMap: Map<string, NodeDetail>; sessions: SessionData[]; globalMacs: Map<string, GlobalMacData> } => {
  const nodeMap = new Map<string, NodeDetail>();
  raw.nodes.forEach(n => {
    // Prefer the new macs array; fall back to wrapping the legacy mac string
    const macs: MacEntry[] = n.macs && n.macs.length > 0
      ? n.macs
      : (n.mac && n.mac !== 'Unknown' ? [{ mac: n.mac, packets_at_node: n.packets, bytes_at_node: n.bytes }] : []);
    nodeMap.set(n.ip, { ip: n.ip, macs, packets: n.packets, bytes: n.bytes });

    // Validation: Sum of all MAC packets_at_node should equal node.packets (Green bar check)
    const sumMacPackets = macs.reduce((sum, m) => sum + m.packets_at_node, 0);
    const sumMacBytes = macs.reduce((sum, m) => sum + m.bytes_at_node, 0);
    if (Math.abs(sumMacPackets - n.packets) > 1) { // Allow 1 packet tolerance for rounding
      console.warn(`⚠️ Data integrity issue for node ${n.ip}:`, {
        node_packets: n.packets,
        sum_of_mac_packets: sumMacPackets,
        difference: sumMacPackets - n.packets,
        message: 'Sum of MAC packets_at_node should equal node.packets'
      });
    }
    if (Math.abs(sumMacBytes - n.bytes) > 100) { // Allow 100 byte tolerance for rounding
      console.warn(`⚠️ Data integrity issue for node ${n.ip}:`, {
        node_bytes: n.bytes,
        sum_of_mac_bytes: sumMacBytes,
        difference: sumMacBytes - n.bytes,
        message: 'Sum of MAC bytes_at_node should equal node.bytes'
      });
    }
  });

  const globalMacs = new Map<string, GlobalMacData>();
  (raw.macs ?? []).forEach(m => {
    globalMacs.set(m.mac, m);
  });

  // Belt-and-suspenders IPv6 filter: backend already drops IPv6 flows in
  // build_topology_data, but if anything (e.g. a malformed v4-mapped address)
  // slips through, recognize it by the presence of ':' in either endpoint and
  // skip it before it reaches vis-network.
  const isIpv4 = (s: BackendLinkData) =>
    !s.src_ip.includes(':') && !s.dst_ip.includes(':');

  const sessions: SessionData[] = (raw.links ?? []).filter(isIpv4).map(s => {
    const endStatus = s.end_status ?? '';
    const e = endStatus.toLowerCase();
    return {
      sessionId:   s.id,
      protocol:    PROTOCOL_MAP[s.protocol] ?? `PROTO_${s.protocol}`,
      sourceIp:    s.src_ip,
      sourcePort:  s.src_port,
      destIp:      s.dst_ip,
      destPort:    s.dst_port,
      startTime:   s.start_time,
      endTime:     s.end_time,
      packets:     s.packets,
      bytes:       s.bytes,
      startStatus: s.start_status ?? '',
      endStatus,
      hasReset:    e.includes('reset'),
      hasTimeout:  e.includes('timeout'),
    };
  });

  return { nodeMap, sessions, globalMacs };
};

const NODE_COLORS = [
  { background: '#00a3ff', border: '#0b1326', highlight: { background: '#00d4ff', border: '#00a3ff' } },
  { background: '#10b981', border: '#0b1326', highlight: { background: '#34d399', border: '#00a3ff' } },
  { background: '#8b5cf6', border: '#0b1326', highlight: { background: '#a78bfa', border: '#00a3ff' } },
  { background: '#f59e0b', border: '#0b1326', highlight: { background: '#fbbf24', border: '#00a3ff' } },
  { background: '#64748b', border: '#0b1326', highlight: { background: '#7c8a9d', border: '#00a3ff' } },
];

interface NetworkTopologyProps {
  onToggleView?: () => void;
  isFullView?: boolean;
}

export function NetworkTopology({ onToggleView, isFullView = false }: NetworkTopologyProps) {
  const containerRef = useRef<HTMLDivElement>(null);
  const networkRef   = useRef<Network | null>(null);
  const nodesDSRef   = useRef<DataSet<any> | null>(null);
  const edgesDSRef   = useRef<DataSet<any> | null>(null);

  const [nodeMap,      setNodeMap]      = useState<Map<string, NodeDetail>>(new Map());
  const [allSessions,  setAllSessions]  = useState<SessionData[]>([]);
  const [globalMacs,   setGlobalMacs]   = useState<Map<string, GlobalMacData>>(new Map());
  const [timeRange,    setTimeRange]    = useState({ start: 0, end: 0 });
  // Slider stores millisecond offset from timeRange.start to avoid float precision issues
  const [sliderMs,     setSliderMs]     = useState(0);
  const [isLoading,    setIsLoading]    = useState(true);
  const [error,        setError]        = useState<string | null>(null);

  const [selectedNodeIp,  setSelectedNodeIp]  = useState<string | null>(null);
  const [selectedEdgeKey, setSelectedEdgeKey] = useState<string | null>(null);
  const [searchQuery,     setSearchQuery]     = useState('');
  const [showSearch,      setShowSearch]      = useState(false);
  const [isDeviceInfoOpen,    setIsDeviceInfoOpen]    = useState(false);
  const [isMacsOpen,          setIsMacsOpen]          = useState(false);
  const [isConnectionsOpen,   setIsConnectionsOpen]   = useState(false);
  const [expandedSessions,    setExpandedSessions]    = useState<Set<string>>(new Set());

  const { activeFileId } = useFileContext();

  // Derived: current timestamp in float Unix seconds
  const currentTimestamp = timeRange.start + sliderMs / 1000;
  const sliderMaxMs = Math.round((timeRange.end - timeRange.start) * 1000);

  // ─── useMemo: filter sessions visible at currentTimestamp → grouped by IP pair ───
  const visibleEdgeGroups = useMemo<Map<string, SessionData[]>>(() => {
    const edgeMap = new Map<string, SessionData[]>();
    for (const session of allSessions) {
      if (session.startTime <= currentTimestamp && session.endTime >= currentTimestamp) {
        const key = [session.sourceIp, session.destIp].sort().join('|');
        if (!edgeMap.has(key)) edgeMap.set(key, []);
        edgeMap.get(key)!.push(session);
      }
    }
    return edgeMap;
  }, [allSessions, currentTimestamp]);

  // ─── useMemo: convert grouped edges to vis-network edge objects ───
  const visEdges = useMemo(() => {
    return Array.from(visibleEdgeGroups.entries()).map(([key, sessions]) => {
      const [ipA, ipB] = key.split('|');
      const totalBytes   = sessions.reduce((s, sess) => s + sess.bytes, 0);
      const hasReset     = sessions.some(s => s.hasReset);
      const hasTimeout   = sessions.some(s => s.hasTimeout);
      const edgeColor    = hasReset ? '#ef4444' : hasTimeout ? '#f97316' : '#00a3ff';
      const highlightCol = hasReset ? '#ff6b6b' : '#00d4ff';
      // Edge width scaled by log of total bytes (2–10px range)
      const width = Math.max(2, Math.min(10, 2 + Math.log2(Math.max(1, totalBytes / 1024))));
      return {
        id: key,
        from: ipA,
        to: ipB,
        width,
        color: { color: edgeColor, highlight: highlightCol, hover: highlightCol },
        label: `${sessions.length}`,
        dashes: hasReset || hasTimeout ? false : [5, 5],
      };
    });
  }, [visibleEdgeGroups]);

  // ─── Fetch topology from backend ───
  useEffect(() => {
    const load = async () => {
      // Check if we even have a file to analyze
      if (!activeFileId) return;

      setIsLoading(true);
      setError(null);

      // Use the centralized config instead of manual string concatenation
      const url = API_ENDPOINTS.TOPOLOGY.ANALYZE;
      console.log('Fetching topology from:', url);

      try {
        const resp = await apiFetch(url, {
          method: 'GET',
          headers: {
            'Accept': 'application/json',
          },
        });

        if (!resp.ok) throw new Error(`HTTP ${resp.status} ${resp.statusText}`);

        const data: BackendResponse = await resp.json();
        console.log('Backend Response:', data);

        // Parse backend data including global MAC registry
        const { nodeMap: nm, sessions, globalMacs: gm } = parseBackendData(data);
        setNodeMap(nm);
        setAllSessions(sessions);
        setGlobalMacs(gm);

        if (sessions.length > 0) {
          const minT = sessions.reduce((m, s) => Math.min(m, s.startTime), Infinity);
          const maxT = sessions.reduce((m, s) => Math.max(m, s.endTime), -Infinity);
          setTimeRange({ start: minT, end: maxT });
          // Set slider to max duration to show all data initially
          setSliderMs(Math.round((maxT - minT) * 1000));
        }
      } catch (err) {
        const msg = err instanceof Error ? err.message : String(err);
        console.error('Topology fetch failed:', msg);
        setError(`CONNECTION ERROR — ${msg}`);
      } finally {
        setIsLoading(false);
      }
    };

    load();
    // Dependency on activeFileId ensures the graph reloads when the file changes
  }, [activeFileId]);


  // ─── Initialize vis-network once with reactive DataSets ───
  useEffect(() => {
    if (!containerRef.current) return;

    nodesDSRef.current = new DataSet([]);
    edgesDSRef.current = new DataSet([]);

    const options = {
      nodes: {
        shape: 'dot',
        size: 28,
        font: { size: 11, color: '#dae2fd', face: 'monospace' },
        borderWidth: 2,
        borderWidthSelected: 3,
      },
      edges: {
        smooth: { enabled: true, type: 'continuous', roundness: 0.5 },
        font: { size: 9, color: '#dae2fd', background: '#0b1326', strokeWidth: 0 },
        arrows: { to: { enabled: false } },
      },
      physics: {
        enabled: true,
        barnesHut: {
          gravitationalConstant: -8000,
          centralGravity: 0.3,
          springLength: 200,
          springConstant: 0.04,
          damping: 0.09,
          avoidOverlap: 0.5,
        },
        stabilization: { iterations: 200, updateInterval: 25 },
      },
      interaction: { dragNodes: true, dragView: true, zoomView: true, hover: false },
      layout: { improvedLayout: true },
    };

    networkRef.current = new Network(
      containerRef.current,
      { nodes: nodesDSRef.current, edges: edgesDSRef.current },
      options
    );

    networkRef.current.on('click', (params) => {
      if (params.nodes.length > 0) {
        setSelectedNodeIp(params.nodes[0] as string);
        setSelectedEdgeKey(null);
      } else if (params.edges.length > 0) {
        setSelectedEdgeKey(params.edges[0] as string);
        setSelectedNodeIp(null);
      } else {
        setSelectedNodeIp(null);
        setSelectedEdgeKey(null);
      }
    });

    return () => {
      networkRef.current?.destroy();
      networkRef.current = null;
    };
  }, []);

  // ─── Sync nodeMap → vis DataSet (only when nodes change) ───
  useEffect(() => {
    if (!nodesDSRef.current) return;
    const allPackets = Array.from(nodeMap.values()).map(n => n.packets);
    const maxPkts = Math.max(1, ...allPackets);
    const nodeArray = Array.from(nodeMap.values()).map((node, idx) => {
      // Size: 12–40px logarithmically scaled by packet count relative to busiest node
      const ratio = node.packets / maxPkts;
      const size  = Math.round(12 + 28 * Math.sqrt(ratio));
      return {
        id:    node.ip,
        label: node.ip,
        size,
        color: NODE_COLORS[idx % NODE_COLORS.length],
      };
    });
    nodesDSRef.current.clear();
    if (nodeArray.length > 0) nodesDSRef.current.add(nodeArray);
  }, [nodeMap]);

  // ─── Sync visEdges → vis DataSet (reactive, no network re-creation) ───
  useEffect(() => {
    if (!edgesDSRef.current) return;
    edgesDSRef.current.clear();
    if (visEdges.length > 0) edgesDSRef.current.add(visEdges);
  }, [visEdges]);

  // Reset panel collapse state when a new element is selected
  useEffect(() => {
    setIsDeviceInfoOpen(false);
    setIsMacsOpen(false);
    setIsConnectionsOpen(false);
    setExpandedSessions(new Set());
  }, [selectedNodeIp, selectedEdgeKey]);

  const handleZoomIn  = () => { if (networkRef.current) networkRef.current.moveTo({ scale: networkRef.current.getScale() * 1.2 }); };
  const handleZoomOut = () => { if (networkRef.current) networkRef.current.moveTo({ scale: networkRef.current.getScale() * 0.8 }); };
  const handleFit     = () => networkRef.current?.fit();

  const toggleSession = (id: string) => {
    setExpandedSessions(prev => {
      const next = new Set(prev);
      next.has(id) ? next.delete(id) : next.add(id);
      return next;
    });
  };

  // Derived panel data
  const selectedNode = selectedNodeIp ? nodeMap.get(selectedNodeIp) ?? null : null;

  const selectedEdgeSessions = selectedEdgeKey
    ? (visibleEdgeGroups.get(selectedEdgeKey) ?? [])
    : [];

  const nodeActiveSessions = useMemo(() => {
    if (!selectedNodeIp) return [];
    return allSessions.filter(s =>
      (s.sourceIp === selectedNodeIp || s.destIp === selectedNodeIp) &&
      s.startTime <= currentTimestamp && s.endTime >= currentTimestamp
    );
  }, [selectedNodeIp, allSessions, currentTimestamp]);

  const [edgeIpA, edgeIpB] = selectedEdgeKey ? selectedEdgeKey.split('|') : ['', ''];
  const edgeTotalBytes   = selectedEdgeSessions.reduce((s, sess) => s + sess.bytes, 0);

  const totalActiveSessions = useMemo(
    () => Array.from(visibleEdgeGroups.values()).reduce((s, arr) => s + arr.length, 0),
    [visibleEdgeGroups]
  );

  // ─── Session card shared renderer ───
  const renderSessionCard = (session: SessionData) => {
    const isExpanded = expandedSessions.has(session.sessionId);
    return (
      <div
        key={session.sessionId}
        className={`bg-[#0b1326] border rounded overflow-hidden ${session.hasReset ? 'border-red-500/40' : 'border-[#31394d]'}`}
      >
        <button
          onClick={() => toggleSession(session.sessionId)}
          className="w-full px-3 py-2 flex items-center justify-between hover:bg-[#060e20] text-[10px]"
        >
          <div className="flex items-center gap-1.5 flex-wrap min-w-0">
            <span className={`font-bold font-mono truncate ${session.hasReset ? 'text-red-400' : 'text-[#00a3ff]'}`}>
              {session.sessionId}
            </span>
            <span className="text-[#dae2fd] font-mono">:{session.sourcePort}→:{session.destPort}</span>
            <span className="bg-[#00a3ff]/20 text-[#00a3ff] px-1.5 py-0.5 rounded text-[9px] font-bold shrink-0">
              {session.protocol}
            </span>
            {session.hasReset && (
              <span className="bg-red-500/20 text-red-400 px-1.5 py-0.5 rounded text-[9px] font-bold shrink-0">RESET</span>
            )}
            {session.hasTimeout && (
              <span className="bg-orange-500/20 text-orange-400 px-1.5 py-0.5 rounded text-[9px] font-bold shrink-0">TIMEOUT</span>
            )}
          </div>
          <ChevronDown className={`w-4 h-4 text-[#88919d] transition-transform shrink-0 ml-1 ${isExpanded ? 'rotate-180' : ''}`} />
        </button>

        {isExpanded && (
          <div className="px-3 pb-3 space-y-2 border-t border-[#31394d]/50">
            {/* 5-tuple */}
            <div className="flex items-center gap-2 bg-[#060e20] p-2 rounded border border-[#31394d]/50 mt-3 text-[10px]">
              <div className="flex-1 text-center">
                <div className="text-[#00a3ff] font-mono font-bold">{session.sourceIp}</div>
                <div className="text-[#64748b] text-[9px]">:{session.sourcePort}</div>
              </div>
              <div className="text-[#64748b]">→</div>
              <div className="flex-1 text-center">
                <div className="text-[#00a3ff] font-mono font-bold">{session.destIp}</div>
                <div className="text-[#64748b] text-[9px]">:{session.destPort}</div>
              </div>
              <span className="bg-[#00a3ff]/20 text-[#00a3ff] px-2 py-1 rounded text-[9px] font-bold ml-2">
                {session.protocol}
              </span>
            </div>

            {/* TCP Status */}
            <div className="grid grid-cols-2 gap-2 text-[9px]">
              <div className="flex flex-col gap-0.5">
                <span className="text-[#64748b]">Start Status</span>
                <span className={`self-start px-1.5 py-0.5 rounded font-bold text-[8px] leading-tight ${getStatusStyle(session.startStatus)}`}>
                  {session.startStatus || '—'}
                </span>
              </div>
              <div className="flex flex-col gap-0.5">
                <span className="text-[#64748b]">End Status</span>
                <span className={`self-start px-1.5 py-0.5 rounded font-bold text-[8px] leading-tight ${getStatusStyle(session.endStatus)}`}>
                  {session.endStatus || '—'}
                </span>
              </div>
              <div>
                <span className="text-[#64748b]">Packets: </span>
                <span className="text-[#dae2fd]">{session.packets.toLocaleString()}</span>
              </div>
              <div>
                <span className="text-[#64748b]">Data: </span>
                <span className="text-[#dae2fd]">{formatBytes(session.bytes)}</span>
              </div>
            </div>

            {/* Duration */}
            <div className="text-[9px] pt-1 border-t border-[#31394d]/50">
              <span className="text-[#64748b]">Duration:</span>
              <div className="text-[#dae2fd] mt-0.5 font-mono">
                {formatTimestampS(session.startTime)} → {formatTimestampS(session.endTime)}
              </div>
            </div>
          </div>
        )}
      </div>
    );
  };

  return (
    <div className="relative h-full bg-[#0b1326] flex flex-col">
      {/* ── Header ── */}
      <div className="h-12 bg-[#060e20]/80 backdrop-blur border-b border-[#31394d] flex items-center justify-between px-4 shrink-0 z-10">
        <div className="flex items-center gap-2">
          <Server className="w-4 h-4 text-[#00a3ff]" />
          <span className="font-bold text-[#dae2fd] text-[11px] tracking-[0.88px]">NETWORK_TOPOLOGY_MAP</span>
          {error ? (
            <span className="ml-2 px-2 py-0.5 border border-red-500/50 rounded text-red-400 text-[9px]">
              CONNECTION ERROR
            </span>
          ) : (
            <span className="ml-2 px-2 py-0.5 border border-[#00a3ff]/50 rounded text-[#00a3ff] text-[9px]">
              {isLoading ? 'LOADING…' : 'LIVE'}
            </span>
          )}
        </div>
        <div className="flex items-center gap-2">
          {showSearch && (
            <input
              type="text"
              placeholder="Search IP…"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              onKeyDown={(e) => {
                if (e.key === 'Enter' && searchQuery.trim()) {
                  const ip = Array.from(nodeMap.keys()).find(k => k.includes(searchQuery.trim()));
                  if (ip && networkRef.current) {
                    networkRef.current.selectNodes([ip]);
                    networkRef.current.focus(ip, { scale: 1.5, animation: true });
                  }
                }
              }}
              className="w-48 h-7 px-2 bg-[#0b1326] border border-[#31394d] rounded text-[#dae2fd] text-[10px] outline-none focus:border-[#00a3ff]"
              autoFocus
            />
          )}
          <button onClick={() => setShowSearch(!showSearch)} className="w-7 h-7 flex items-center justify-center rounded hover:bg-[#171f33] text-[#88919d]">
            {showSearch ? <X className="w-4 h-4" /> : <Search className="w-4 h-4" />}
          </button>
          <button onClick={handleZoomIn}  className="w-7 h-7 flex items-center justify-center rounded hover:bg-[#171f33] text-[#88919d]"><ZoomIn  className="w-4 h-4" /></button>
          <button onClick={handleZoomOut} className="w-7 h-7 flex items-center justify-center rounded hover:bg-[#171f33] text-[#88919d]"><ZoomOut className="w-4 h-4" /></button>
          <button onClick={handleFit}     className="w-7 h-7 flex items-center justify-center rounded hover:bg-[#171f33] text-[#88919d]"><Locate  className="w-4 h-4" /></button>
          <button onClick={onToggleView}  className="w-7 h-7 flex items-center justify-center rounded hover:bg-[#171f33] text-[#88919d]" title={isFullView ? 'Return to Dashboard' : 'Topology Only'}>
            <Maximize2 className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* ── Graph Canvas ── */}
      <div className="flex-1 relative overflow-hidden">
        {isLoading && (
          <div className="absolute inset-0 flex items-center justify-center z-20 bg-[#0b1326]/80">
            <div className="flex items-center gap-3 text-[#dae2fd]">
              <Loader2 className="w-5 h-5 animate-spin text-[#00a3ff]" />
              <span className="text-[11px] font-bold tracking-wider">ANALYZING PCAP…</span>
            </div>
          </div>
        )}

        {!isLoading && error && (
          <div className="absolute inset-0 flex flex-col items-center justify-center z-20 bg-[#0b1326]">
            <div className="border border-red-500/40 bg-red-500/10 rounded-lg px-8 py-6 max-w-sm text-center space-y-3">
              <div className="text-red-400 text-[11px] font-bold tracking-[1px]">CONNECTION ERROR</div>
              <div className="text-[#88919d] text-[10px] font-mono break-all">{error.replace('CONNECTION ERROR — ', '')}</div>
              <div className="text-[#64748b] text-[9px]">
                Ensure the backend is running at<br />
                <span className="text-[#dae2fd] font-mono">{BACKEND_URL}</span>
              </div>
            </div>
          </div>
        )}

        <div ref={containerRef} className="w-full h-full" />

        {/* ── Node Info Panel ── */}
        {selectedNode && (
          <div className="absolute top-4 right-4 w-96 bg-[#060e20] border border-[#31394d] rounded-lg overflow-hidden max-h-[calc(100%-2rem)] flex flex-col z-20">
            <div className="bg-[#00a3ff]/10 border-b border-[#31394d] px-4 py-3 flex items-center justify-between shrink-0">
              <div className="flex items-center gap-2">
                <Server className="w-4 h-4 text-[#00a3ff]" />
                <span className="text-[#dae2fd] font-bold text-[11px] tracking-[0.88px]">NODE_INFO</span>
              </div>
              <button onClick={() => setSelectedNodeIp(null)} className="text-[#88919d] hover:text-[#dae2fd]">
                <X className="w-4 h-4" />
              </button>
            </div>

            <div className="flex-1 overflow-y-auto">
              <Collapsible open={isDeviceInfoOpen} onOpenChange={setIsDeviceInfoOpen}>
                <CollapsibleTrigger className="w-full px-4 py-3 border-b border-[#31394d] flex items-center justify-between hover:bg-[#0b1326]/50">
                  <span className="text-[#64748b] text-[10px] font-bold tracking-[0.8px]">DEVICE INFO</span>
                  <ChevronDown className={`w-4 h-4 text-[#64748b] transition-transform ${isDeviceInfoOpen ? 'rotate-180' : ''}`} />
                </CollapsibleTrigger>
                <CollapsibleContent>
                  <div className="p-4 border-b border-[#31394d] text-[11px] space-y-2">
                    <div className="flex items-center justify-between">
                      <span className="text-[#64748b]">IP Address:</span>
                      <span className="text-[#00a3ff] font-mono font-bold">{selectedNode.ip}</span>
                    </div>
                    <div className="flex items-center justify-between">
                      <span className="text-[#64748b]">Packets:</span>
                      <span className="text-[#00a3ff] font-bold">{selectedNode.packets.toLocaleString()}</span>
                    </div>
                    <div className="flex items-center justify-between">
                      <span className="text-[#64748b]">Bytes:</span>
                      <span className="text-[#00a3ff] font-bold">{formatBytes(selectedNode.bytes)}</span>
                    </div>
                  </div>
                </CollapsibleContent>
              </Collapsible>

              {/* ── Physical Interfaces (MAC) ── */}
              {(() => {
                const validMacs = selectedNode.macs.filter(m => m.mac && m.mac !== 'Unknown');
                const disabled  = validMacs.length === 0;
                const totalPkts = validMacs.reduce((s, m) => s + m.packets_at_node, 0) || 1;
                return (
                  <Collapsible
                    open={isMacsOpen && !disabled}
                    onOpenChange={v => !disabled && setIsMacsOpen(v)}
                  >
                    <CollapsibleTrigger
                      disabled={disabled}
                      className={`w-full px-4 py-3 border-b border-[#31394d] flex items-center justify-between
                        ${disabled ? 'opacity-40 cursor-not-allowed' : 'hover:bg-[#0b1326]/50 cursor-pointer'}`}
                    >
                      <div className="flex items-center gap-2">
                        <span className="text-[#64748b] text-[10px] font-bold tracking-[0.8px]">PHYSICAL INTERFACES</span>
                        <span className={`px-1.5 py-0.5 rounded text-[9px] font-bold
                          ${disabled ? 'bg-[#1e293b] text-[#64748b]' : 'bg-[#00a3ff]/15 text-[#00a3ff]'}`}>
                          {disabled ? '0' : validMacs.length}
                        </span>
                      </div>
                      {!disabled && (
                        <ChevronDown className={`w-4 h-4 text-[#64748b] transition-transform ${isMacsOpen ? 'rotate-180' : ''}`} />
                      )}
                    </CollapsibleTrigger>
                    <CollapsibleContent>
                      <div className="bg-[#060e20] border-b border-[#31394d]">
                        {validMacs.map((entry, idx) => {
                          const globalMac = globalMacs.get(entry.mac);

                          // Calculate RAW percentages - no capping
                          const macPktPct = globalMac && globalMac.total_packets > 0
                            ? Math.round((entry.packets_at_node / globalMac.total_packets) * 100)
                            : 0;
                          const macBytePct = globalMac && globalMac.total_bytes > 0
                            ? Math.round((entry.bytes_at_node / globalMac.total_bytes) * 100)
                            : 0;
                          const nodePktPct = selectedNode.packets > 0
                            ? Math.round((entry.packets_at_node / selectedNode.packets) * 100)
                            : 0;
                          const nodeBytePct = selectedNode.bytes > 0
                            ? Math.round((entry.bytes_at_node / selectedNode.bytes) * 100)
                            : 0;

                          // Debug mode: detect overflow and log raw values
                          const hasOverflow = macPktPct > 100 || macBytePct > 100 || nodePktPct > 100 || nodeBytePct > 100;
                          if (hasOverflow) {
                            console.error('🔴 PERCENTAGE OVERFLOW DETECTED:', {
                              mac: entry.mac,
                              ip: selectedNode.ip,
                              'MAC packets (Blue)': {
                                numerator: entry.packets_at_node,
                                denominator: globalMac?.total_packets ?? 0,
                                percentage: macPktPct,
                                expected: 'packets_at_node ≤ total_packets'
                              },
                              'MAC bytes (Blue)': {
                                numerator: entry.bytes_at_node,
                                denominator: globalMac?.total_bytes ?? 0,
                                percentage: macBytePct,
                                expected: 'bytes_at_node ≤ total_bytes'
                              },
                              'Node packets (Green)': {
                                numerator: entry.packets_at_node,
                                denominator: selectedNode.packets,
                                percentage: nodePktPct,
                                expected: 'packets_at_node ≤ node.packets'
                              },
                              'Node bytes (Green)': {
                                numerator: entry.bytes_at_node,
                                denominator: selectedNode.bytes,
                                percentage: nodeBytePct,
                                expected: 'bytes_at_node ≤ node.bytes'
                              },
                              issue: macPktPct > 100 || macBytePct > 100
                                ? 'Blue bar overflow: This MAC\'s traffic at this node exceeds its global total. Check if global totals are being calculated correctly.'
                                : 'Green bar overflow: This MAC\'s traffic exceeds the node\'s total traffic. Check if node totals include all MAC traffic.'
                            });
                          }

                          return (
                            <div
                              key={idx}
                              className="px-4 py-2.5 border-b border-[#1e293b] last:border-0 hover:bg-[#0b1326]/60 transition-colors"
                            >
                              {/* MAC address with traffic stats */}
                              <div className="flex items-center justify-between gap-3">
                                <div className="font-mono text-[11px] text-[#dae2fd] tracking-wide">{entry.mac}</div>
                                <div className="flex items-center gap-2.5">
                                  <span className="text-[#64748b] text-[9px] font-mono shrink-0">
                                    {entry.packets_at_node.toLocaleString()} pkts
                                  </span>
                                  <span className="text-[#64748b] text-[9px] font-mono shrink-0">
                                    {formatBytes(entry.bytes_at_node)}
                                  </span>
                                </div>
                              </div>

                              {/* Traffic share metrics */}
                              <div className="mt-2 space-y-2">
                                {/* % of MAC's GLOBAL traffic - Packets */}
                                <div
                                  className="space-y-0.5 group relative"
                                  title="This MAC's packets at this node divided by this MAC's total packets across all IP addresses in the network"
                                >
                                  <div className="flex items-center justify-between">
                                    <span className="text-[#64748b] text-[9px]">% of MAC's traffic - Packets</span>
                                    <span className={`text-[9px] font-bold ${macPktPct > 100 ? 'text-red-500' : 'text-[#00a3ff]'}`}>
                                      {macPktPct}% <span className="text-[#475569]">of {globalMac?.total_packets.toLocaleString() ?? '0'}</span>
                                    </span>
                                  </div>
                                  <div className="flex-1 h-1 bg-[#1e293b] rounded-full overflow-hidden">
                                    <div
                                      className={`h-full rounded-full ${macPktPct > 100 ? 'bg-red-500/60' : 'bg-[#00a3ff]/60'}`}
                                      style={{ width: `${Math.min(100, macPktPct)}%` }}
                                    />
                                  </div>
                                </div>

                                {/* % of MAC's GLOBAL traffic - Bytes */}
                                <div
                                  className="space-y-0.5 group relative"
                                  title="This MAC's bytes at this node divided by this MAC's total bytes across all IP addresses in the network"
                                >
                                  <div className="flex items-center justify-between">
                                    <span className="text-[#64748b] text-[9px]">% of MAC's traffic - Bytes</span>
                                    <span className={`text-[9px] font-bold ${macBytePct > 100 ? 'text-red-500' : 'text-[#00a3ff]'}`}>
                                      {macBytePct}% <span className="text-[#475569]">of {formatBytes(globalMac?.total_bytes ?? 0)}</span>
                                    </span>
                                  </div>
                                  <div className="flex-1 h-1 bg-[#1e293b] rounded-full overflow-hidden">
                                    <div
                                      className={`h-full rounded-full ${macBytePct > 100 ? 'bg-red-500/60' : 'bg-[#00a3ff]/60'}`}
                                      style={{ width: `${Math.min(100, macBytePct)}%` }}
                                    />
                                  </div>
                                </div>

                                {/* % of node's total traffic - Packets */}
                                <div
                                  className="space-y-0.5 group relative"
                                  title="This MAC's packets at this node divided by the total packets sent/received by this IP address"
                                >
                                  <div className="flex items-center justify-between">
                                    <span className="text-[#64748b] text-[9px]">% of Node traffic - Packets</span>
                                    <span className={`text-[9px] font-bold ${nodePktPct > 100 ? 'text-red-500' : 'text-[#10b981]'}`}>
                                      {nodePktPct}% <span className="text-[#475569]">of {selectedNode.packets.toLocaleString()}</span>
                                    </span>
                                  </div>
                                  <div className="flex-1 h-1 bg-[#1e293b] rounded-full overflow-hidden">
                                    <div
                                      className={`h-full rounded-full ${nodePktPct > 100 ? 'bg-red-500/60' : 'bg-[#10b981]/60'}`}
                                      style={{ width: `${Math.min(100, nodePktPct)}%` }}
                                    />
                                  </div>
                                </div>

                                {/* % of node's total traffic - Bytes */}
                                <div
                                  className="space-y-0.5 group relative"
                                  title="This MAC's bytes at this node divided by the total bytes sent/received by this IP address"
                                >
                                  <div className="flex items-center justify-between">
                                    <span className="text-[#64748b] text-[9px]">% of Node traffic - Bytes</span>
                                    <span className={`text-[9px] font-bold ${nodeBytePct > 100 ? 'text-red-500' : 'text-[#10b981]'}`}>
                                      {nodeBytePct}% <span className="text-[#475569]">of {formatBytes(selectedNode.bytes)}</span>
                                    </span>
                                  </div>
                                  <div className="flex-1 h-1 bg-[#1e293b] rounded-full overflow-hidden">
                                    <div
                                      className={`h-full rounded-full ${nodeBytePct > 100 ? 'bg-red-500/60' : 'bg-[#10b981]/60'}`}
                                      style={{ width: `${Math.min(100, nodeBytePct)}%` }}
                                    />
                                  </div>
                                </div>
                              </div>
                            </div>
                          );
                        })}
                      </div>
                    </CollapsibleContent>
                  </Collapsible>
                );
              })()}

              <Collapsible open={isConnectionsOpen} onOpenChange={setIsConnectionsOpen}>
                <CollapsibleTrigger className="w-full px-4 py-3 border-b border-[#31394d] flex items-center justify-between hover:bg-[#0b1326]/50">
                  <span className="text-[#64748b] text-[10px] font-bold tracking-[0.8px]">
                    ACTIVE SESSIONS ({nodeActiveSessions.length})
                  </span>
                  <ChevronDown className={`w-4 h-4 text-[#64748b] transition-transform ${isConnectionsOpen ? 'rotate-180' : ''}`} />
                </CollapsibleTrigger>
                <CollapsibleContent>
                  <div className="p-4 space-y-3">
                    {nodeActiveSessions.length === 0 ? (
                      <div className="text-[#64748b] text-[10px] text-center py-4">No active sessions at this timestamp</div>
                    ) : (
                      nodeActiveSessions.map(renderSessionCard)
                    )}
                  </div>
                </CollapsibleContent>
              </Collapsible>
            </div>
          </div>
        )}

        {/* ── Edge Sessions Panel ── */}
        {selectedEdgeKey && selectedEdgeSessions.length > 0 && (
          <div className="absolute top-4 right-4 w-96 bg-[#060e20] border border-[#31394d] rounded-lg overflow-hidden max-h-[calc(100%-2rem)] flex flex-col z-20">
            <div className="bg-[#00a3ff]/10 border-b border-[#31394d] px-4 py-3 flex items-center justify-between shrink-0">
              <div className="flex items-center gap-2">
                <Server className="w-4 h-4 text-[#00a3ff]" />
                <span className="text-[#dae2fd] font-bold text-[11px] tracking-[0.88px]">CONNECTION_SESSIONS</span>
              </div>
              <button onClick={() => setSelectedEdgeKey(null)} className="text-[#88919d] hover:text-[#dae2fd]">
                <X className="w-4 h-4" />
              </button>
            </div>

            <div className="px-4 py-3 border-b border-[#31394d] text-[11px] shrink-0">
              <div className="text-[#64748b] text-[9px]">Connection:</div>
              <div className="text-[#00a3ff] font-mono font-bold mt-1">{edgeIpA} ↔ {edgeIpB}</div>
              <div className="flex items-center gap-4 mt-2 text-[10px]">
                <span className="text-[#64748b]">Sessions: <span className="text-[#00a3ff] font-bold">{selectedEdgeSessions.length}</span></span>
                <span className="text-[#64748b]">Total: <span className="text-[#dae2fd]">{formatBytes(edgeTotalBytes)}</span></span>
                {selectedEdgeSessions.some(s => s.hasReset) && (
                  <span className="bg-red-500/20 text-red-400 px-1.5 py-0.5 rounded text-[9px] font-bold">HAS RESET</span>
                )}
              </div>
            </div>

            <div className="flex-1 overflow-y-auto p-4 space-y-3">
              {selectedEdgeSessions.map(renderSessionCard)}
            </div>
          </div>
        )}
      </div>

      {/* ── Timestamp Slider ── */}
      <div className="h-[5.5rem] bg-[#060e20] border-t border-[#31394d] px-6 py-3 shrink-0">
        <div className="flex items-center gap-4 mb-2">
          <span className="text-[#64748b] text-[10px] font-bold tracking-[0.8px]">TIMELINE</span>
          <span className="text-[#00a3ff] text-[11px] font-mono">{formatTimestampS(currentTimestamp)}</span>
          <span className="text-[#64748b] text-[9px] font-mono">±1ms precision</span>
        </div>
        <div className="flex items-center gap-4">
          <span className="text-[#64748b] text-[9px] font-mono shrink-0">{formatTimestampS(timeRange.start)}</span>
          <input
            type="range"
            min={0}
            max={sliderMaxMs}
            value={sliderMs}
            step={1}
            onChange={(e) => setSliderMs(Number(e.target.value))}
            className="flex-1 h-1 bg-[#1e293b] rounded-lg appearance-none cursor-pointer
              [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-3 [&::-webkit-slider-thumb]:h-3
              [&::-webkit-slider-thumb]:rounded-full [&::-webkit-slider-thumb]:bg-[#00a3ff] [&::-webkit-slider-thumb]:cursor-pointer
              [&::-moz-range-thumb]:w-3 [&::-moz-range-thumb]:h-3 [&::-moz-range-thumb]:rounded-full
              [&::-moz-range-thumb]:bg-[#00a3ff] [&::-moz-range-thumb]:border-0 [&::-moz-range-thumb]:cursor-pointer"
          />
          <span className="text-[#64748b] text-[9px] font-mono shrink-0">{formatTimestampS(timeRange.end)}</span>
        </div>
        <div className="mt-1 text-[9px] text-[#64748b]">
          Active edges: <span className="text-[#00a3ff] font-bold">{visibleEdgeGroups.size}</span>
          <span className="ml-4">Active sessions: <span className="text-[#dae2fd] font-bold">{totalActiveSessions}</span></span>
        </div>
      </div>
    </div>
  );
}
