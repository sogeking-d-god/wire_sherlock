import { useCallback, useEffect, useRef, useState } from 'react';
import { Send, Sparkles, Loader2, CheckCircle2, XCircle } from 'lucide-react';
import { toast } from 'sonner';
import { useLocation, useNavigate } from 'react-router';

import { streamChat, type ChatEvent, type ToolCallEventData } from '../../api/chat';
import { useViewMode, type ViewMode } from '../context/ViewModeContext';
import { useFileContext } from '../context/FileContext';
import { useChatDrawer } from '../context/ChatDrawerContext';

interface ChatMsg {
  role: 'user' | 'assistant';
  content: string;
  /** Persistent trace of tool calls that ran during this assistant turn. */
  trace?: ToolTraceEntry[];
}

interface ToolTraceEntry {
  name: string;
  status: 'started' | 'completed' | 'failed';
  message?: string;
}

interface ActiveTool {
  name: string;
  arguments: Record<string, unknown>;
}

const TOOL_LABELS: Record<string, string> = {
  find_http_attacks: 'AGENT IS SCANNING HTTP SIGNATURES VIA C-ENGINE…',
  find_anomalies:    'AGENT IS COMPUTING ANOMALIES VIA C-ENGINE…',
  cluster_anomalies: 'AGENT IS CLUSTERING ANOMALIES VIA C-ENGINE…',
  list_metrics:      'AGENT IS LISTING METRIC DEFINITIONS…',
};

/** Map each backend tool to the dashboard view that visualizes its results. */
const TOOL_TO_VIEW: Record<string, ViewMode> = {
  find_http_attacks: 'topology',
  find_anomalies:    'metrics',
  cluster_anomalies: 'metrics',
};

/** Auto-navigation is only allowed if the user hasn't manually steered to a
 *  specific analysis view (topology/metrics) since the turn started. From
 *  `dashboard` we're free to focus a sub-view; from `topology`/`metrics` we
 *  assume the user is intentionally there and leave them alone. */
const AUTO_NAV_FROM: ReadonlyArray<ViewMode> = ['dashboard'];

const INITIAL_GREETING: ChatMsg = {
  role: 'assistant',
  content:
    "Hi — I'm the WireSherlock agent. Ask me about HTTP attacks, traffic spikes, or any patterns in the loaded PCAP, and I'll query the C-engine and summarize what I find.",
};

export function AIAssistant() {
  const { viewMode, setViewMode, setHighlight } = useViewMode();
  const { activeFileName } = useFileContext();
  const { open: openChatDrawer } = useChatDrawer();
  const navigate = useNavigate();
  const location = useLocation();

  const [messages, setMessages] = useState<ChatMsg[]>([INITIAL_GREETING]);
  const [input, setInput] = useState('');
  const [isStreaming, setIsStreaming] = useState(false);
  const [activeTool, setActiveTool] = useState<ActiveTool | null>(null);

  // Refs that mustn't trigger re-renders.
  const abortRef = useRef<AbortController | null>(null);
  const scrollAnchorRef = useRef<HTMLDivElement | null>(null);
  // Snapshot viewMode at the start of each turn so auto-nav reads a stable value.
  const turnStartViewRef = useRef<ViewMode>(viewMode);
  // Snapshot the route at turn start. The dashboard route is the only place
  // where ViewMode is rendered; if the agent picks a sub-view from anywhere
  // else (e.g. /files), we navigate there before flipping the viewMode.
  const turnStartPathRef = useRef<string>(location.pathname);

  // Auto-scroll to the bottom on any change to messages or the active tool card.
  useEffect(() => {
    scrollAnchorRef.current?.scrollIntoView({ behavior: 'smooth', block: 'end' });
  }, [messages, activeTool]);

  // Cancel any in-flight stream when the component unmounts.
  useEffect(() => {
    return () => {
      abortRef.current?.abort();
    };
  }, []);

  const appendDeltaToLastAssistant = useCallback((delta: string) => {
    setMessages(prev => {
      if (prev.length === 0) return prev;
      const last = prev[prev.length - 1];
      if (last.role !== 'assistant') return prev;
      const updated: ChatMsg = { ...last, content: last.content + delta };
      return [...prev.slice(0, -1), updated];
    });
  }, []);

  const appendTraceToLastAssistant = useCallback((entry: ToolTraceEntry) => {
    setMessages(prev => {
      if (prev.length === 0) return prev;
      const last = prev[prev.length - 1];
      if (last.role !== 'assistant') return prev;
      const updated: ChatMsg = { ...last, trace: [...(last.trace ?? []), entry] };
      return [...prev.slice(0, -1), updated];
    });
  }, []);

  const handleEvent = useCallback((ev: ChatEvent) => {
    switch (ev.event) {
      case 'token':
        appendDeltaToLastAssistant(ev.data.delta);
        break;

      case 'tool_call': {
        const td: ToolCallEventData = ev.data;
        if (td.status === 'started') {
          setActiveTool({ name: td.name, arguments: td.arguments });
          // Auto-navigate to the relevant view — but only if the user hasn't
          // manually steered to topology/metrics since the turn started.
          const target = TOOL_TO_VIEW[td.name];
          if (target && AUTO_NAV_FROM.includes(turnStartViewRef.current)) {
            // ViewMode is only rendered on the /dashboard route. If the user
            // submitted from /files, navigate there first so flipping the
            // viewMode actually shows something.
            if (turnStartPathRef.current !== '/dashboard') {
              navigate('/dashboard');
            }
            setViewMode(target);
            setHighlight(td.arguments);
          }
        } else if (td.status === 'completed') {
          appendTraceToLastAssistant({ name: td.name, status: 'completed' });
          setActiveTool(null);
        } else {
          appendTraceToLastAssistant({
            name: td.name, status: 'failed', message: td.message,
          });
          setActiveTool(null);
        }
        break;
      }

      case 'done':
        if (ev.data.reason !== 'complete') {
          toast.error(`Agent stopped: ${ev.data.reason}`, {
            description: ev.data.detail,
          });
        }
        break;
    }
  }, [appendDeltaToLastAssistant, appendTraceToLastAssistant, setViewMode, setHighlight, navigate]);

  const submit = useCallback(async () => {
    const prompt = input.trim();
    if (!prompt || isStreaming) return;
    if (!activeFileName) {
      toast.error('Select a PCAP file first', {
        description: 'The agent needs an active session to query the C-engine.',
      });
      return;
    }

    // Cancel any in-flight stream defensively (shouldn't happen — button is disabled).
    abortRef.current?.abort();
    const ctrl = new AbortController();
    abortRef.current = ctrl;

    // Optimistic UI: drop the user's message into the transcript and clear the
    // input BEFORE awaiting the network. This way the message survives even if
    // the SSE connection takes a moment to open.
    setMessages(prev => [
      ...prev,
      { role: 'user', content: prompt },
      { role: 'assistant', content: '', trace: [] },
    ]);
    setInput('');
    setIsStreaming(true);
    setActiveTool(null);
    // Make sure the drawer is open — submitting from a collapsed drawer (e.g.
    // via a future hotkey) shouldn't hide the streaming reply from the user.
    openChatDrawer();
    turnStartViewRef.current = viewMode;
    turnStartPathRef.current = location.pathname;

    try {
      for await (const ev of streamChat(prompt, { signal: ctrl.signal })) {
        handleEvent(ev);
      }
    } catch (err) {
      if ((err as Error).name === 'AbortError') {
        // user-initiated cancel — silent
      } else {
        const msg = err instanceof Error ? err.message : 'Unknown error';
        toast.error('Chat failed', { description: msg });
        appendDeltaToLastAssistant(`\n\n[error: ${msg}]`);
      }
    } finally {
      setIsStreaming(false);
      setActiveTool(null);
      abortRef.current = null;
    }
  }, [
    input, isStreaming, activeFileName, viewMode,
    handleEvent, appendDeltaToLastAssistant,
    openChatDrawer, location.pathname,
  ]);

  const onKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      submit();
    }
  };

  return (
    <div className="h-full bg-[#0f172a] flex flex-col border-l border-[#31394d]">
      {/* Header */}
      <div className="h-12 bg-[#060e20] border-b border-[#31394d] flex items-center justify-between px-4 shrink-0">
        <div className="flex items-center gap-2">
          <div className={`w-2 h-2 rounded-full ${isStreaming ? 'bg-[#00a3ff] animate-pulse' : 'bg-[#10b981]'}`} />
          <Sparkles className="w-4 h-4 text-[#00a3ff]" />
          <span className="font-bold text-[#dae2fd] text-[11px] tracking-[0.88px]">
            NET_VISOR_AI_ASSISTANT
          </span>
        </div>
        <span className="text-[9px] text-[#64748b]">
          {isStreaming ? 'STREAMING' : 'IDLE'}
        </span>
      </div>

      {/* Session info */}
      <div className="px-4 py-2 border-b border-[#31394d]/50 shrink-0">
        <div className="text-[#64748b] text-[11px] font-mono truncate">
          PCAP // {activeFileName ?? '<none selected>'}
        </div>
      </div>

      {/* Messages — natural top-down order; sentinel at the bottom for auto-scroll. */}
      <div className="flex-1 overflow-y-auto px-4 py-4 space-y-4">
        {messages.map((msg, idx) => {
          const isLastAssistant =
            idx === messages.length - 1 && msg.role === 'assistant' && isStreaming;
          return (
            <MessageBubble
              key={idx}
              msg={msg}
              activeTool={isLastAssistant ? activeTool : null}
              isStreaming={isLastAssistant}
            />
          );
        })}
        <div ref={scrollAnchorRef} />
      </div>

      {/* Input */}
      <div className="border-t border-[#31394d] p-4 shrink-0">
        <div className="flex gap-2">
          <input
            type="text"
            value={input}
            onChange={e => setInput(e.target.value)}
            onKeyDown={onKeyDown}
            disabled={isStreaming}
            placeholder={isStreaming ? 'Agent is working…' : 'Ask about the pcap data…'}
            className="flex-1 bg-[#171f33] border border-[#31394d] rounded px-3 py-2 text-[13px] text-[#dae2fd] placeholder:text-[#3f4852] outline-none focus:border-[#00a3ff] disabled:opacity-50"
          />
          <button
            onClick={submit}
            disabled={isStreaming || !input.trim()}
            className="w-10 h-10 bg-[#00a3ff] rounded flex items-center justify-center hover:bg-[#0090e0] transition-colors disabled:opacity-40 disabled:cursor-not-allowed"
          >
            {isStreaming ? (
              <Loader2 className="w-4 h-4 text-[#0b1326] animate-spin" />
            ) : (
              <Send className="w-4 h-4 text-[#0b1326]" />
            )}
          </button>
        </div>
        <div className="mt-2 text-[9px] text-[#64748b]">
          AI Content. Current view: {viewMode.toUpperCase()}
        </div>
      </div>
    </div>
  );
}

/* ------------------------------------------------------------------------- */

interface MessageBubbleProps {
  msg: ChatMsg;
  /** Only set on the currently-streaming assistant message. */
  activeTool: ActiveTool | null;
  isStreaming: boolean;
}

function MessageBubble({ msg, activeTool, isStreaming }: MessageBubbleProps) {
  const isUser = msg.role === 'user';
  return (
    <div className={`flex flex-col gap-1 ${isUser ? 'items-end' : 'items-start'}`}>
      <div
        className={`max-w-[90%] rounded-lg p-3 ${
          isUser
            ? 'bg-[#00a3ff] text-[#0b1326]'
            : 'bg-[#171f33] text-[#dae2fd] border border-[#31394d]'
        }`}
      >
        <div className="text-[12px] leading-relaxed whitespace-pre-wrap min-h-[1em]">
          {msg.content}
          {isStreaming && (
            <span className="inline-block w-2 h-3 ml-0.5 bg-[#00a3ff] animate-pulse align-middle" />
          )}
        </div>

        {/* Live tool spinner — emitted on tool_call status=started, cleared on completed/failed. */}
        {activeTool && (
          <div className="mt-2 flex items-start gap-2 rounded border border-[#00a3ff]/40 bg-[#00a3ff]/10 px-3 py-2">
            <Loader2 className="w-4 h-4 mt-0.5 animate-spin text-[#00a3ff] shrink-0" />
            <div className="flex flex-col min-w-0">
              <span className="text-[10px] font-bold tracking-[0.6px] text-[#00a3ff]">
                {TOOL_LABELS[activeTool.name] ?? `RUNNING ${activeTool.name.toUpperCase()}…`}
              </span>
              {Object.keys(activeTool.arguments).length > 0 && (
                <span className="text-[10px] text-[#64748b] font-mono truncate">
                  args: {JSON.stringify(activeTool.arguments)}
                </span>
              )}
            </div>
          </div>
        )}

        {/* Persistent trace of tools that already finished this turn. */}
        {msg.trace && msg.trace.length > 0 && (
          <div className="mt-2 space-y-1">
            {msg.trace.map((t, i) => (
              <div
                key={i}
                className={`flex items-center gap-1.5 text-[10px] font-mono ${
                  t.status === 'failed' ? 'text-[#f43f5e]' : 'text-[#10b981]'
                }`}
              >
                {t.status === 'failed' ? (
                  <XCircle className="w-3 h-3" />
                ) : (
                  <CheckCircle2 className="w-3 h-3" />
                )}
                <span>ran {t.name}</span>
                {t.message && <span className="text-[#64748b]">— {t.message}</span>}
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
}
