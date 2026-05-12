import { LayoutDashboard, Network, BarChart3, MessageSquare, FileText, LogOut } from 'lucide-react';
import imgUserProfileAvatar from "../../imports/Html→Body/1d11ad8865371c71a99fe1ea4b3f949c317248db.png";
import { useFileContext } from '../context/FileContext';
import { toast } from 'sonner';


interface SideNavBarProps {
  onOpenModal: (view: 'topology' | 'metrics' | 'assistant' | null) => void;
  currentView: 'dashboard' | 'topology' | 'metrics' | 'assistant';
  onDashboardClick: () => void;
}

export function SideNavBar({ onOpenModal, currentView, onDashboardClick }: SideNavBarProps) {
  const { activeFileName } = useFileContext();

  const navItems = [
    { icon: LayoutDashboard, label: 'DASHBOARD', view: 'dashboard' as const, action: null },
    { icon: Network, label: 'TOPOLOGY', view: 'topology' as const, action: 'topology' as const },
    { icon: BarChart3, label: 'METRICS', view: 'metrics' as const, action: 'metrics' as const },
    { icon: MessageSquare, label: 'AI_ASSISTANT', view: 'assistant' as const, action: 'assistant' as const },
  ];

  const handleClick = (item: typeof navItems[0]) => {
    if (item.view === 'dashboard') {
      onDashboardClick();
      return;
    }

    if (item.action && !activeFileName) {
      toast.error("Please select a PCAP file first", {
        description: "Go to the Archive to select a file for analysis."
      });
      return;
    }

    if (item.action) {
      onOpenModal(item.action);
    }
  };

  return (
    <div className="bg-[#0f172a] w-[80px] border-r border-[#31394d] flex flex-col items-center">
      {/* Header - Operator Info */}
      <div className="w-full border-b border-[#31394d]/50 py-4 px-3 flex flex-col items-center gap-2">
        <div className="w-10 h-10 rounded border border-[#3f4852] bg-[#222a3d] overflow-hidden">
          <img src={imgUserProfileAvatar} alt="" className="w-full h-full object-cover mix-blend-luminosity opacity-80" />
        </div>
        <div className="text-center">
          <div className="text-[#98cbff] text-[11px] font-bold tracking-[0.88px] truncate w-full">
            OPERATOR_01
          </div>
          <div className="text-[#88919d] text-[13px] truncate w-full">
            ADMIN_NOD
          </div>
        </div>
      </div>

      {/* Main Nav Items */}
      <div className="flex-1 w-full py-2">
        {navItems.map((item) => {
          const isDisabled = !!item.action && !activeFileName;

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
                currentView === item.view
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