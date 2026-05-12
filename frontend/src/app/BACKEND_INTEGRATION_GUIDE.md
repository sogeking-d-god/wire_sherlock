# WireSherlock Backend Integration Guide

## Overview
This guide explains how to integrate the WireSherlock frontend with your FastAPI backend for real-time PCAP analysis.

## Backend Data Requirements

### Current Backend Response Format
```python
{
  "nodes": {
    "192.168.1.5": {
      "ip": "192.168.1.5",
      "macs": ["00:1A:2B:3C:4D:5E"],
      "total_packets": 45230
    },
    ...
  },
  "links": [
    {
      "id": "192.168.1.5-10.0.0.1-6-0",
      "src_ip": "192.168.1.5",
      "dst_ip": "10.0.0.1",
      "start_time": 1620000000000,
      "end_time": 1620001000000,
      "protocol": 6
    },
    ...
  ]
}
```

### ⚠️ Missing Data Fields for Full Visualization

To make the topology visualization fully functional, your backend should include these additional fields:

#### **1. Port Information for Sessions**
```python
class SessionDetails(BaseModel):
    id: str
    src_ip: str
    dst_ip: str
    src_port: int  # ← ADD THIS
    dst_port: int  # ← ADD THIS
    start_time: int
    end_time: int
    protocol: int
    packets: int  # ← ADD THIS (for session statistics)
    bytes: int  # ← ADD THIS (for session statistics)
```

#### **2. Session Closing Status**
```python
closing_status: str  # ← ADD THIS
# Possible values: "gracefully", "reset", "timeout", "not_closed"
```

Logic for determining closing status:
- **"gracefully"**: TCP session with FIN flags from both sides
- **"reset"**: TCP session closed with RST flag
- **"timeout"**: Session ended without proper closure
- **"not_closed"**: Session still active at end of capture

#### **3. Time Range Information**
```python
class TopologyResponse(BaseModel):
    nodes: dict[str, NodeDetails]
    links: list[SessionDetails]
    time_range: TimeRange  # ← ADD THIS

class TimeRange(BaseModel):
    start: int  # Unix timestamp in milliseconds
    end: int    # Unix timestamp in milliseconds
```

#### **4. Node Statistics (Optional but Recommended)**
```python
class NodeDetails(BaseModel):
    ip: str
    macs: list[str]
    total_packets: int
    first_seen: int  # ← ADD THIS (Unix timestamp)
    last_seen: int   # ← ADD THIS (Unix timestamp)
    total_sessions: int  # ← ADD THIS (count of sessions)
```

## Frontend Implementation

### Data Transformation Functions

Add these functions before the `NetworkTopology` component:

```typescript
// Transform backend response to frontend format
const transformBackendData = (
  backendData: BackendTopologyResponse
): { nodes: NodeDetails[]; edges: EdgeData[]; timeRange: { start: number; end: number } } => {
  // Create IP to ID mapping
  const ipToId = new Map<string, number>();
  const nodesList: NodeDetails[] = [];
  
  // Transform nodes
  Object.entries(backendData.nodes).forEach(([ip, nodeData], index) => {
    const nodeId = index + 1;
    ipToId.set(ip, nodeId);
    
    // Calculate time range from sessions involving this node
    const nodeSessions = backendData.links.filter(
      link => link.src_ip === ip || link.dst_ip === ip
    );
    
    const firstSeen = nodeSessions.length > 0 
      ? Math.min(...nodeSessions.map(s => s.start_time))
      : Date.now();
    const lastSeen = nodeSessions.length > 0
      ? Math.max(...nodeSessions.map(s => s.end_time))
      : Date.now();
    
    nodesList.push({
      id: nodeId,
      ip: nodeData.ip,
      macs: nodeData.macs || [],
      firstSeen,
      lastSeen,
      totalSessions: nodeSessions.length,
      totalPackets: nodeData.total_packets || 0,
    });
  });
  
  // Group sessions by node pair
  const edgeMap = new Map<string, SessionData[]>();
  
  backendData.links.forEach(link => {
    const srcId = ipToId.get(link.src_ip);
    const dstId = ipToId.get(link.dst_ip);
    
    if (!srcId || !dstId) return;
    
    // Create consistent edge key (smaller ID first)
    const edgeKey = srcId < dstId ? `${srcId}-${dstId}` : `${dstId}-${srcId}`;
    
    if (!edgeMap.has(edgeKey)) {
      edgeMap.set(edgeKey, []);
    }
    
    // Determine closing status
    let closingStatus: 'gracefully' | 'reset' | 'timeout' | 'not closed' = 'not closed';
    if (link.closing_status) {
      closingStatus = link.closing_status as any;
    } else if (link.end_time < Date.now() - 60000) {
      closingStatus = 'timeout';
    }
    
    edgeMap.get(edgeKey)!.push({
      sessionId: link.id,
      protocol: getProtocolName(link.protocol),
      sourceIp: link.src_ip,
      sourcePort: link.src_port || 0,
      destIp: link.dst_ip,
      destPort: link.dst_port || 0,
      startTime: link.start_time,
      endTime: link.end_time,
      packets: link.packets || 0,
      bytes: link.bytes || 0,
      closingStatus,
    });
  });
  
  // Create edges array
  const edgesList: EdgeData[] = [];
  edgeMap.forEach((sessions, edgeKey) => {
    const [from, to] = edgeKey.split('-').map(Number);
    edgesList.push({ from, to, sessions });
  });
  
  // Calculate time range
  const allTimes = backendData.links.flatMap(link => [link.start_time, link.end_time]);
  const timeRange = {
    start: allTimes.length > 0 ? Math.min(...allTimes) : Date.now() - 300000,
    end: allTimes.length > 0 ? Math.max(...allTimes) : Date.now(),
  };
  
  return { nodes: nodesList, edges: edgesList, timeRange };
};
```

### Fetching Logic

Add this `useEffect` hook at the beginning of the `NetworkTopology` component (after state declarations):

```typescript
// Fetch topology data from backend
useEffect(() => {
  const fetchTopologyData = async () => {
    setIsLoading(true);
    setError(null);
    
    try {
      const params = new URLSearchParams({
        pcap_path: PCAP_PATH,
      });
      
      const response = await fetch(
        `${BACKEND_URL}/api/topology/analyze?${params}`,
        {
          method: 'GET',
          headers: {
            'Content-Type': 'application/json',
          },
        }
      );
      
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      
      const backendData: BackendTopologyResponse = await response.json();
      
      // Transform backend data to frontend format
      const transformed = transformBackendData(backendData);
      
      setTopologyData({
        nodes: transformed.nodes,
        edges: transformed.edges,
      });
      
      // Update time range
      setTimeRange(transformed.timeRange);
      setCurrentTimestamp(transformed.timeRange.end);
      
      setIsLoading(false);
    } catch (err) {
      console.error('Failed to fetch topology data:', err);
      setError(err instanceof Error ? err.message : 'Failed to load topology data');
      setIsLoading(false);
      
      // Fallback to mock data
      console.warn('Using fallback mock data');
      setTopologyData({
        nodes: nodeDetailsData,
        edges: allEdgesData,
      });
    }
  };
  
  fetchTopologyData();
}, []);  // Empty dependency array = fetch once on mount
```

### Update Visualization Logic

Replace the hardcoded data references in the visualization `useEffect`:

```typescript
// BEFORE (using hardcoded data):
const nodes = [
  { id: 1, label: '192.168.1.5', group: 'server' },
  // ...
];

// AFTER (using fetched data):
const nodesData = topologyData?.nodes || nodeDetailsData;
const edgesData = topologyData?.edges || allEdgesData;

const nodes = nodesData.map(node => ({
  id: node.id,
  label: node.ip,
  group: determineNodeGroup(node),  // Function to determine group
}));
```

### Node Grouping Logic

Add this function to intelligently group nodes:

```typescript
const determineNodeGroup = (node: NodeDetails): 'server' | 'client' | 'router' => {
  // Logic to determine node type based on behavior
  if (node.totalSessions > 20) return 'router';  // High connection count
  if (node.ip.includes('192.168') || node.ip.includes('10.0')) {
    return node.totalSessions > 5 ? 'server' : 'client';
  }
  return 'client';
};
```

### Loading and Error UI

Add this to the graph container section:

```typescript
{/* Loading State */}
{isLoading && (
  <div className="absolute inset-0 flex items-center justify-center bg-[#0b1326]/80 backdrop-blur z-30">
    <div className="flex flex-col items-center gap-3">
      <Loader2 className="w-8 h-8 text-[#00a3ff] animate-spin" />
      <span className="text-[#dae2fd] text-[11px]">Loading topology...</span>
    </div>
  </div>
)}

{/* Error State */}
{error && !isLoading && (
  <div className="absolute top-4 left-1/2 -translate-x-1/2 bg-[#fb923c]/20 border border-[#fb923c] rounded px-4 py-2 flex items-center gap-2 z-30">
    <AlertCircle className="w-4 h-4 text-[#fb923c]" />
    <span className="text-[#fb923c] text-[10px]">{error}</span>
  </div>
)}
```

## Backend Updates Summary

### Priority 1 (Required for basic functionality)
- [x] Nodes dictionary with IP, MACs, total_packets ✅ Already implemented
- [x] Links array with id, src_ip, dst_ip, start_time, end_time, protocol ✅ Already implemented
- [ ] **Add src_port and dst_port to SessionDetails**
- [ ] **Add packets and bytes to SessionDetails**

### Priority 2 (Enhanced visualization)
- [ ] **Add closing_status to SessionDetails** ("gracefully" | "reset" | "timeout" | "not_closed")
- [ ] **Add time_range to TopologyResponse** (start and end timestamps)

### Priority 3 (Optional statistics)
- [ ] Add first_seen and last_seen to NodeDetails
- [ ] Add total_sessions count to NodeDetails

## Testing

### 1. Test Backend Connection
```bash
curl "http://localhost:8000/api/topology/analyze?pcap_path=/home/ronen/wire_sherlock/pcap_files/regular_pcap_file.pcap"
```

### 2. Enable CORS on Backend
Make sure your FastAPI backend has CORS enabled:

```python
from fastapi.middleware.cors import CORSMiddleware

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # In production, specify your frontend URL
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
```

### 3. Check Browser Console
Open browser DevTools → Console to see:
- Fetch requests
- Data transformation logs
- Any errors

## Fallback Strategy

The frontend includes automatic fallback to mock data if:
- Backend is unreachable
- API returns an error
- Data format is invalid

This ensures the app remains functional during development.

## Future Enhancements

Once basic integration works, consider adding:
1. **Real-time updates**: WebSocket connection for live data
2. **Multiple PCAP support**: UI for selecting different PCAP files
3. **Filtering**: Backend endpoints for filtered data (by protocol, time range, etc.)
4. **Pagination**: For large network captures with thousands of nodes
5. **Export**: Download topology data as JSON/CSV

---

**Next Steps:**
1. Update your Python backend with the missing fields
2. Apply the transformation functions to the frontend
3. Test with a real PCAP file
4. Iterate based on data quality and performance
