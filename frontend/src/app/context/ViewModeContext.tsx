import { createContext, useCallback, useContext, useState, ReactNode } from 'react';

export type ViewMode = 'dashboard' | 'topology' | 'metrics';

/** Optional payload the agent attaches when navigating — e.g. the time range
 *  of an anomaly, so the metrics view could later draw a highlight band. */
export type ViewHighlight = Record<string, unknown> | null;

interface ViewModeContextType {
  viewMode: ViewMode;
  setViewMode: (v: ViewMode) => void;
  highlight: ViewHighlight;
  setHighlight: (h: ViewHighlight) => void;
}

const ViewModeContext = createContext<ViewModeContextType | undefined>(undefined);

export function ViewModeProvider({
  children,
  initial = 'dashboard',
}: {
  children: ReactNode;
  initial?: ViewMode;
}) {
  const [viewMode, setViewModeState] = useState<ViewMode>(initial);
  const [highlight, setHighlightState] = useState<ViewHighlight>(null);

  const setViewMode = useCallback((v: ViewMode) => setViewModeState(v), []);
  const setHighlight = useCallback((h: ViewHighlight) => setHighlightState(h), []);

  return (
    <ViewModeContext.Provider value={{ viewMode, setViewMode, highlight, setHighlight }}>
      {children}
    </ViewModeContext.Provider>
  );
}

export function useViewMode(): ViewModeContextType {
  const ctx = useContext(ViewModeContext);
  if (ctx === undefined) {
    throw new Error('useViewMode must be used within a ViewModeProvider');
  }
  return ctx;
}
