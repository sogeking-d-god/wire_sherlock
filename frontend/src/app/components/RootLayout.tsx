import { Outlet } from 'react-router';
import { useState } from 'react';
import { TopNavBar } from './TopNavBar';
import AuthView from './AuthView'; // ייבוא של קומפוננטת ה-Login שיצרנו

export function RootLayout() {
  // בדיקה אקטיבית האם קיים טוקן מאוחסן בדפדפן
  const [token, setToken] = useState<string | null>(localStorage.getItem('token'));

  // פונקציה שתועבר ל-AuthView ותופעל ברגע שההתחברות/הרשמה הצליחה
  const handleAuthSuccess = (jwtToken: string) => {
    localStorage.setItem('token', jwtToken);
    setToken(jwtToken); // מעדכן את הסטייט וגורם לרינדור מחדש של המסך כמחובר
  };

  // פונקציית התנתקות (אפשר להעביר אותה ל-TopNavBar במידת הצורך)
  const handleLogout = () => {
    localStorage.removeItem('token');
    setToken(null);
  };

  // תנאי הגנה: אם אין אסימון אבטחה, המערכת מציגה אך ורק את מסך ה-Login
  if (!token) {
    return <AuthView onAuthSuccess={handleAuthSuccess} />;
  }

  // אם המשתמש מחובר, האפליקציה מתנהגת כרגיל ומאפשרת ניווט
  return (
    <div className="bg-[#0b1326] h-screen w-screen flex flex-col overflow-hidden">
      {/* מעבירים את פונקציית ההתנתקות ל-Navbar כדי שהכפתור שם יעבוד */}
      <TopNavBar onLogout={handleLogout} />
      <div className="flex flex-1 overflow-hidden">
        <Outlet />
      </div>
    </div>
  );
}