import { Outlet } from 'react-router';
import { TopNavBar } from './TopNavBar';

export function RootLayout() {
  return (
    <div className="bg-[#0b1326] h-screen w-screen flex flex-col overflow-hidden">
      <TopNavBar />
      <div className="flex flex-1 overflow-hidden">
        <Outlet />
      </div>
    </div>
  );
}
