import { Search, LogOut, Archive, Save, Circle } from 'lucide-react';
import { NavLink } from 'react-router';
import { useState, useEffect } from 'react';

import { useFileContext } from '../context/FileContext';
import { API_ENDPOINTS } from '../../config';


export function TopNavBar() {
  const { activeFileName, setActiveFile } = useFileContext();


  return (
    <div className="bg-[#060e20] h-[56px] border-b border-[#31394d] flex items-center justify-between px-4">
      {/* Brand and Search */}
      <div className="flex items-center gap-6">
        <div className="font-black text-[#00a3ff] text-[18px] tracking-[1.8px]">
          WIRESHERLOCK
        </div>
        <div className="relative">
          <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-[13.5px] h-[13.5px] text-[#3f4852]" />
          <input
            type="text"
            placeholder="SEARCH_QUERY..."
            className="bg-[#171f33] border border-[#31394d] rounded px-3 pl-9 py-1.5 text-[13px] text-[#3f4852] w-[256px] outline-none focus:border-[#00a3ff]"
          />
        </div>
        {/* Session Status */}
        {activeFileName ? (
          <div className="flex items-center gap-2 px-3 py-1 rounded bg-[#10b981]/10 border border-[#10b981]/30">
            <Circle className="w-2 h-2 fill-[#10b981] text-[#10b981]" />
            <span className="text-[11px] font-bold text-[#10b981] uppercase tracking-wide">
              Active: {activeFileName}
            </span>
          </div>
        ) : (
          <div className="flex items-center gap-2 px-3 py-1 rounded bg-[#64748b]/10 border border-[#64748b]/30">
            <Circle className="w-2 h-2 fill-[#64748b] text-[#64748b]" />
            <span className="text-[11px] font-bold text-[#64748b] uppercase tracking-wide">
              No File Selected
            </span>
          </div>
        )}
      </div>

      {/* Navigation Links */}
      <div className="flex items-center h-full">
        <NavLink to="/files" className={({ isActive }) => `h-full px-4 flex items-center gap-2 ${isActive ? 'border-b-2 border-[#00a3ff]' : ''}`}>
          {({ isActive }) => (
            <>
              <Archive className={`w-4 h-4 ${isActive ? 'text-[#00a3ff]' : 'text-[#64748b]'}`} />
              <span className={`font-bold text-[12px] uppercase ${isActive ? 'text-[#00a3ff]' : 'text-[#64748b] hover:text-[#00a3ff]'}`}>
                ARCHIVE
              </span>
            </>
          )}
        </NavLink>
        <NavLink
          to="/dashboard"
          className={({ isActive }) =>
            `h-full px-4 flex items-center ${
              isActive ? 'border-b-2 border-[#00a3ff]' : ''
            }`
          }
        >
          {({ isActive }) => (
            <span
              className={`font-bold text-[12px] tracking-[-0.6px] uppercase ${
                isActive ? 'text-[#00a3ff]' : 'text-[#64748b] hover:text-[#00a3ff]'
              }`}
            >
              DASHBOARD
            </span>
          )}
        </NavLink>
        <div className="h-full px-4 flex items-center">
          <span className="font-bold text-[#64748b] text-[12px] tracking-[-0.6px] uppercase">SESSIONS</span>
        </div>
        <div className="h-full px-4 flex items-center">
          <span className="font-bold text-[#64748b] text-[12px] tracking-[-0.6px] uppercase">PACKETS</span>
        </div>
        <div className="h-full px-4 flex items-center">
          <span className="font-bold text-[#64748b] text-[12px] tracking-[-0.6px] uppercase">STATISTICS</span>
        </div>
      </div>

      {/* Actions */}
      <div className="flex items-center gap-3">
        <button className="flex items-center gap-2 px-3 py-1.5 rounded hover:bg-[#171f33] text-[#88919d] hover:text-[#00a3ff]">
          <Save className="w-4 h-4" />
          <span className="font-bold text-[11px] tracking-[0.88px]">SAVE</span>
        </button>
        <div className="h-6 w-px bg-[#31394d]" />
        <button className="flex items-center gap-2 px-3 py-1.5 rounded hover:bg-[#171f33] text-[#88919d] hover:text-[#f43f5e]">
          <LogOut className="w-4 h-4" />
          <span className="font-bold text-[11px] tracking-[0.88px]">LOGOUT</span>
        </button>
      </div>
    </div>
  );
}