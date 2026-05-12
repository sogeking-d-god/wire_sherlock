import { useState, useEffect } from 'react';
import { NetworkTopology } from './NetworkTopology';
import { TrafficChart } from './TrafficChart';
import { AIAssistant } from './AIAssistant';
import { SideNavBar } from './SideNavBar';
import { FileText, Activity, Archive, AlertCircle, Loader2 } from 'lucide-react';
import { useFileContext } from '../context/FileContext';
import { useNavigate } from 'react-router';
import { Button } from './ui/button';

import { API_ENDPOINTS } from '../../config';

type ViewMode = 'dashboard' | 'topology' | 'metrics' | 'assistant';

export function AnalysisView() {
  const [viewMode, setViewMode] = useState<ViewMode>('dashboard');
  const { activeFileName, activeFileId, setActiveFile, clearActiveFile } = useFileContext();
  const navigate = useNavigate();
  const [isValidatingSession, setIsValidatingSession] = useState(true);
  const [sessionInvalid, setSessionInvalid] = useState(false);

  // STEP 3: Validate active session on mount (after POST /api/pcap/select was called)
  useEffect(() => {
    const syncSession = async () => {
      try {
        const response = await fetch(API_ENDPOINTS.PCAP.SESSION);
        const data = await response.json();

        if (data.active) {
          setActiveFile(data.file_id || data.filename, data.filename);
        } else if (activeFileId) {
          await fetch(API_ENDPOINTS.PCAP.SELECT(activeFileId), { method: 'POST' });
        }
      } catch (error) {
        console.error("Session validation failed:", error);
        setSessionInvalid(true);
      } finally {
        setIsValidatingSession(false);
      }
    };

    syncSession();
  }, [activeFileId]);

  const handleViewChange = (view: 'topology' | 'metrics' | 'assistant' | null) => {
    if (view === null) {
      setViewMode('dashboard');
    } else {
      setViewMode(view);
    }
  };

  const handleTopologyToggle = () => {
    if (viewMode === 'topology') {
      setViewMode('dashboard');
    } else {
      setViewMode('topology');
    }
  };

  const handleMetricsToggle = () => {
    if (viewMode === 'metrics') {
      setViewMode('dashboard');
    } else {
      setViewMode('metrics');
    }
  };

  return (
    <>
      <SideNavBar
        onOpenModal={handleViewChange}
        currentView={viewMode}
        onDashboardClick={() => setViewMode('dashboard')}
      />
      <div className="flex-1 flex flex-col overflow-hidden">
        {/* Status Bar / Breadcrumb */}
        <div className="bg-[#060e20] border-b border-[#31394d] px-6 py-3 flex items-center gap-3">
          <Activity className="w-4 h-4 text-[#00a3ff]" />
          <span className="text-[11px] font-bold text-[#64748b] uppercase tracking-[1.1px]">
            Current Session:
          </span>
          <div className="flex items-center gap-2">
            <FileText className="w-4 h-4 text-[#10b981]" />
            <span className="text-[13px] font-medium text-[#10b981]">
              {activeFileName || 'No file selected'}
            </span>
          </div>
        </div>

        {/* Main Content */}
        <div className="flex-1 flex overflow-hidden">
        {isValidatingSession || sessionInvalid ? (
          // Session Validation Overlay
          <div className="flex-1 flex items-center justify-center bg-[#0b1326]">
            <div className="text-center max-w-md px-8">
              <div className="w-20 h-20 mx-auto mb-6 rounded-full bg-[#171f33] flex items-center justify-center">
                {sessionInvalid ? (
                  <AlertCircle className="w-10 h-10 text-[#f43f5e]" />
                ) : (
                  <Loader2 className="w-10 h-10 text-[#00a3ff] animate-spin" />
                )}
              </div>
              <h2 className="text-[24px] font-black text-[#e2e8f0] mb-3 tracking-[1.2px]">
                {sessionInvalid ? 'NO ACTIVE SESSION' : 'VALIDATING SESSION'}
              </h2>
              <p className="text-[14px] text-[#64748b] mb-6">
                {sessionInvalid
                  ? 'No active session found. Redirecting to Archive...'
                  : 'Checking active PCAP session...'}
              </p>
            </div>
          </div>
        ) : !activeFileId ? (
          // Empty State when no file is selected
          <div className="flex-1 flex items-center justify-center bg-[#0b1326]">
            <div className="text-center max-w-md px-8">
              <div className="w-20 h-20 mx-auto mb-6 rounded-full bg-[#171f33] flex items-center justify-center">
                <AlertCircle className="w-10 h-10 text-[#64748b]" />
              </div>
              <h2 className="text-[24px] font-black text-[#e2e8f0] mb-3 tracking-[1.2px]">
                NO ACTIVE SESSION
              </h2>
              <p className="text-[14px] text-[#64748b] mb-6">
                Please select a PCAP file from the Archive to start network analysis.
              </p>
              <Button
                onClick={() => navigate('/files')}
                className="bg-[#00a3ff] hover:bg-[#0090e0] text-white gap-2"
              >
                <Archive className="w-4 h-4" />
                Go to Archive
              </Button>
            </div>
          </div>
        ) : viewMode === 'dashboard' ? (
          <>
            {/* Left side: Network view and chart */}
            <div className="flex-1 flex flex-col border-r border-[#31394d]">
              {/* Network Topology Map - ~60% */}
              <div className="h-[60%] border-b border-[#31394d]">
                <NetworkTopology onToggleView={handleTopologyToggle} isFullView={false} />
              </div>
              {/* Traffic Metrics Chart - ~40% */}
              <div className="h-[40%]">
                <TrafficChart onToggleView={handleMetricsToggle} isFullView={false} />
              </div>
            </div>
            {/* Right side: AI Assistant Chat */}
            <div className="w-[380px]">
              <AIAssistant />
            </div>
          </>
        ) : null}

        {activeFileId && viewMode === 'topology' && (
          <div className="flex-1">
            <NetworkTopology onToggleView={handleTopologyToggle} isFullView={true} />
          </div>
        )}

        {activeFileId && viewMode === 'metrics' && (
          <div className="flex-1">
            <TrafficChart onToggleView={handleMetricsToggle} isFullView={true} />
          </div>
        )}

        {activeFileId && viewMode === 'assistant' && (
          <div className="flex-1">
            <AIAssistant />
          </div>
        )}

        </div>
      </div>
    </>
  );
}
