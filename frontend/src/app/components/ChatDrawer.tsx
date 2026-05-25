import { ChevronRight, MessageSquare, X } from 'lucide-react';

import { AIAssistant } from './AIAssistant';
import { useChatDrawer } from '../context/ChatDrawerContext';

/**
 * Persistent right-side chat drawer.
 *
 * Lives at the layout level — never unmounts on route or view changes, so the
 * SSE stream survives the agent calling setViewMode('topology') mid-turn and
 * the user's message + streaming tokens are never lost.
 *
 * Collapsed state keeps the <AIAssistant> mounted (display:none rather than
 * conditional render) so in-flight requests continue and chat history is
 * preserved when the user re-opens it.
 */
export function ChatDrawer() {
  const { isOpen, open, close } = useChatDrawer();

  return (
    <>
      {/* Collapsed handle — a thin always-visible tab on the right edge. */}
      {!isOpen && (
        <button
          onClick={open}
          className="absolute top-1/2 right-0 -translate-y-1/2 z-20
                     flex flex-col items-center gap-2
                     bg-[#060e20] border border-[#31394d] border-r-0
                     rounded-l-md px-2 py-3
                     text-[#88919d] hover:text-[#00a3ff] hover:bg-[#0f172a]
                     transition-colors"
          title="Open AI Assistant"
        >
          <MessageSquare className="w-4 h-4" />
          <ChevronRight className="w-4 h-4 rotate-180" />
        </button>
      )}

      {/* Drawer body — kept mounted; toggled with width + display so React
          state, the SSE stream, and any timers all survive collapse. */}
      <div
        className={`shrink-0 transition-[width] duration-200 ease-out ${
          isOpen ? 'w-[380px]' : 'w-0'
        }`}
        aria-hidden={!isOpen}
      >
        <div
          className="h-full relative"
          style={{ display: isOpen ? 'block' : 'none' }}
        >
          {/* Close button overlaid on the drawer header. */}
          <button
            onClick={close}
            className="absolute top-2 right-2 z-10 p-1 rounded
                       text-[#64748b] hover:text-[#f43f5e] hover:bg-[#171f33]"
            title="Collapse AI Assistant"
          >
            <X className="w-4 h-4" />
          </button>
          <AIAssistant />
        </div>
      </div>
    </>
  );
}
