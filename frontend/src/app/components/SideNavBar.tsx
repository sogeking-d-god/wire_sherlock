import { LayoutDashboard, Network, BarChart3, MessageSquare, FileText, LogOut } from 'lucide-react';
import { useFileContext } from '../context/FileContext';
import { useAuth } from '../context/AuthContext';
import imgUserProfileAvatar from '../../assets/user.png';
import { toast } from 'sonner';


interface SideNavBarProps {
  onOpenModal: (view: 'topology' | 'metrics' | null) => void;
  currentView: 'dashboard' | 'topology' | 'metrics';
  onDashboardClick: () => void;
  /** Toggles the global chat drawer (lifted out of viewMode — chat is now a
   *  persistent overlay that never unmounts on view changes). */
  onAssistantClick: () => void;
}

type NavKind =
  | { kind: 'view'; view: 'dashboard' | 'topology' | 'metrics' | null }
  | { kind: 'drawer' };

export function SideNavBar({
  onOpenModal, currentView, onDashboardClick, onAssistantClick,
}: SideNavBarProps) {
  const { activeFileName } = useFileContext();
  const { username } = useAuth();
  const displayName = (username ?? 'GUEST').toUpperCase();

  const navItems: Array<{
    icon: typeof LayoutDashboard;
    label: string;
    nav: NavKind;
    requiresFile: boolean;
  }> = [
    { icon: LayoutDashboard, label: 'DASHBOARD',    nav: { kind: 'view', view: 'dashboard' }, requiresFile: false },
    { icon: Network,         label: 'TOPOLOGY',     nav: { kind: 'view', view: 'topology'  }, requiresFile: true  },
    { icon: BarChart3,       label: 'METRICS',      nav: { kind: 'view', view: 'metrics'   }, requiresFile: true  },
    { icon: MessageSquare,   label: 'AI_ASSISTANT', nav: { kind: 'drawer' },                    requiresFile: true  },
  ];

  const handleClick = (item: typeof navItems[number]) => {
    if (item.requiresFile && !activeFileName) {
      toast.error("Please select a PCAP file first", {
        description: "Go to the Archive to select a file for analysis."
      });
      return;
    }
    if (item.nav.kind === 'drawer') {
      onAssistantClick();
      return;
    }
    if (item.nav.view === 'dashboard') {
      onDashboardClick();
      return;
    }
    onOpenModal(item.nav.view);
  };

  return (
    <div className="bg-[#0f172a] w-[80px] border-r border-[#31394d] flex flex-col items-center">
      {/* Header - Operator Info */}
      <div className="w-full border-b border-[#31394d]/50 py-4 px-3 flex flex-col items-center gap-2">
        <div className="w-10 h-10 rounded border border-[#3f4852] bg-[#222a3d] overflow-hidden">
          <img src={imgUserProfileAvatar} alt="" className="w-full h-full object-cover mix-blend-luminosity opacity-80" />
        </div>
        <div className="text-center w-full px-1">
          <div
            className="text-[#98cbff] text-[11px] font-bold tracking-[0.88px] truncate w-full"
            title={username ?? undefined}
          >
            {displayName}
          </div>
          <div className="text-[#88919d] text-[10px] tracking-wide">
            OPERATOR
          </div>
        </div>
      </div>

      {/* Main Nav Items */}
      <div className="flex-1 w-full py-2">
        {navItems.map((item) => {
          const isDisabled = item.requiresFile && !activeFileName;
          // The drawer toggle never carries "active" state — it's a global
          // overlay, not a viewMode. Only view-kind items light up.
          const isActive =
            item.nav.kind === 'view' && currentView === item.nav.view;

          return (
            <button
              key={item.label}
              onClick={() => handleClick(item)}
              disabled={isDisabled}
              className={`w-full py-4 flex flex-col items-center gap-1 border-r-2 transition-all ${
                isDisabled
                  ? 'opacity-20 cursor-not-allowed filter grayscale'
                  : ''
              } ${
                isActive
                  ? 'border-[#00a3ff] bg-[rgba(0,163,255,0.1)] text-[#00a3ff]'
                  : 'border-transparent text-[#3f4852] hover:bg-[#171f33] hover:text-[#88919d]'
              }`}
            >
              <item.icon className="w-5 h-5" />
              <span className="text-[10px] font-medium tracking-[-0.25px]">
                {item.label}
              </span>
            </button>
          );
        })}
      </div>

      {/* Footer */}
      <div className="w-full border-t border-[#31394d]/50 py-2">
        <button className="w-full py-3 flex flex-col items-center gap-1 text-[#3f4852] hover:bg-[#171f33]">
          <FileText className="w-4 h-4" />
          <span className="text-[9px] font-medium tracking-[-0.225px]">SYSTEM_LOGS</span>
        </button>
        <button className="w-full py-3 flex flex-col items-center gap-1 text-[#3f4852] hover:bg-[#171f33]"
          onClick={() => toast.info("Logout not implemented yet")}>
          <LogOut className="w-4 h-4" />
          <span className="text-[9px] font-medium tracking-[-0.225px]">LOGOUT</span>
        </button>
      </div>
    </div>
  );
}