import { useEffect, useState } from 'react';
import { NetworkTopology } from './NetworkTopology';
import { TrafficChart } from './TrafficChart';
import { SideNavBar } from './SideNavBar';
import { FileText, Activity, Archive, AlertCircle, Loader2 } from 'lucide-react';
import { useFileContext } from '../context/FileContext';
import { useViewMode } from '../context/ViewModeContext';
import { useChatDrawer } from '../context/ChatDrawerContext';
import { useNavigate } from 'react-router';
import { Button } from './ui/button';

import { apiFetch } from '../../api/client';
import { API_ENDPOINTS } from '../../config';

// ViewModeProvider was lifted to RootLayout so the global ChatDrawer can read
// it. AnalysisView is now a plain consumer.
export function AnalysisView() {
  const { viewMode, setViewMode } = useViewMode();
  const { activeFileName, activeFileId, setActiveFile, clearActiveFile } = useFileContext();
  const { toggle: toggleChatDrawer } = useChatDrawer();
  const navigate = useNavigate();
  const [isValidatingSession, setIsValidatingSession] = useState(true);
  const [sessionInvalid, setSessionInvalid] = useState(false);

  // Validate the cached PCAP selection against the BACKEND's view of this user's
  // session. Backend is authoritative — if it says "no active session" we MUST
  // drop the stale frontend pick instead of POSTing /select with it (which is
  // how user B used to inherit user A's PCAP).
  useEffect(() => {
    const syncSession = async () => {
      try {
        const res = await apiFetch(API_ENDPOINTS.PCAP.SESSION);
        const data = await res.json();

        if (data.active) {
          setActiveFile(data.file_id || data.filename, data.filename);
        } else {
          // Server has no session for this user — drop any cached selection so
          // the next file pick is explicit and we don't auto-replay user A's
          // selection for user B.
          clearActiveFile();
        }
      } catch (error) {
        console.error('Session validation failed:', error);
        setSessionInvalid(true);
      } finally {
        setIsValidatingSession(false);
      }
    };

    syncSession();
    // Intentionally NOT depending on activeFileId — we want to trust the
    // server's answer about THIS user's session, not loop on the cached id.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const handleViewChange = (view: 'topology' | 'metrics' | null) => {
    setViewMode(view === null ? 'dashboard' : view);
  };

  const handleTopologyToggle = () => {
    setViewMode(viewMode === 'topology' ? 'dashboard' : 'topology');
  };

  const handleMetricsToggle = () => {
    setViewMode(viewMode === 'metrics' ? 'dashboard' : 'metrics');
  };

  return (
    <>
      <SideNavBar
        onOpenModal={handleViewChange}
        currentView={viewMode}
        onDashboardClick={() => setViewMode('dashboard')}
        onAssistantClick={toggleChatDrawer}
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
            <div className="flex-1 flex flex-col">
              <div className="h-[60%] border-b border-[#31394d]">
                <NetworkTopology onToggleView={handleTopologyToggle} isFullView={false} />
              </div>
              <div className="h-[40%]">
                <TrafficChart onToggleView={handleMetricsToggle} isFullView={false} />
              </div>
            </div>
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
        </div>
      </div>
    </>
  );
}
