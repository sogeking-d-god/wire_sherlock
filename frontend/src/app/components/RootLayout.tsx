import { Outlet } from 'react-router';
import { useCallback, useEffect, useState } from 'react';
import { Loader2 } from 'lucide-react';

import { TopNavBar } from './TopNavBar';
import AuthView from './AuthView';
import { ChatDrawer } from './ChatDrawer';

import { apiFetch, apiJson } from '../../api/client';
import { API_ENDPOINTS } from '../../config';
import { AuthProvider } from '../context/AuthContext';
import { useFileContext } from '../context/FileContext';
import { ViewModeProvider } from '../context/ViewModeContext';
import { ChatDrawerProvider } from '../context/ChatDrawerContext';

type AuthState = 'checking' | 'authed' | 'anon';

interface MePayload {
  user_id: string;
  username?: string;
}

export function RootLayout() {
  const [authState, setAuthState] = useState<AuthState>('checking');
  const [username, setUsername] = useState<string | null>(null);
  const [userId, setUserId] = useState<string | null>(null);
  // Anti-leak: any time we transition to 'anon' (initial 401, logout, expiry),
  // wipe the cached PCAP selection so the next login doesn't auto-attach to it.
  const { clearActiveFile } = useFileContext();

  const goAnonymous = useCallback(() => {
    setAuthState('anon');
    setUsername(null);
    setUserId(null);
    clearActiveFile();
  }, [clearActiveFile]);

  const probeAuth = useCallback(async () => {
    try {
      const me = await apiJson<MePayload>(API_ENDPOINTS.AUTH.ME);
      setUserId(me.user_id);
      setUsername(me.username ?? me.user_id);
      setAuthState('authed');
    } catch {
      goAnonymous();
    }
  }, [goAnonymous]);

  useEffect(() => {
    probeAuth();
  }, [probeAuth]);

  const handleAuthSuccess = useCallback(async () => {
    // The login response carried {username}, but we re-probe /auth/me to keep
    // a single source of truth and pick up the same fields the refresh path uses.
    await probeAuth();
  }, [probeAuth]);

  const handleLogout = useCallback(async () => {
    try {
      await apiFetch(API_ENDPOINTS.AUTH.LOGOUT, { method: 'POST' });
    } catch {
      // Even if the call fails (e.g. cookie already expired), drop to anon.
    }
    goAnonymous();
  }, [goAnonymous]);

  if (authState === 'checking') {
    return (
      <div className="min-h-screen bg-[#0b1326] flex items-center justify-center">
        <Loader2 className="w-8 h-8 text-[#00a3ff] animate-spin" />
      </div>
    );
  }

  if (authState === 'anon') {
    return <AuthView onAuthSuccess={handleAuthSuccess} />;
  }

  return (
    <AuthProvider username={username} userId={userId}>
      {/* ViewModeProvider and ChatDrawerProvider live at the layout level so
          the AIAssistant — mounted as a global drawer below — never unmounts
          when the agent navigates between Topology/Metrics/Dashboard. */}
      <ViewModeProvider initial="dashboard">
        <ChatDrawerProvider initialOpen={false}>
          <div className="bg-[#0b1326] h-screen w-screen flex flex-col overflow-hidden">
            <TopNavBar onLogout={handleLogout} />
            {/* `relative` anchors the ChatDrawer's collapsed-handle button
                (position:absolute) to this container instead of the viewport. */}
            <div className="flex flex-1 overflow-hidden relative">
              <Outlet />
              <ChatDrawer />
            </div>
          </div>
        </ChatDrawerProvider>
      </ViewModeProvider>
    </AuthProvider>
  );
}
