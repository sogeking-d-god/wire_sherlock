import React, { useState } from 'react';
import { Mail, Lock, User, ShieldAlert, ArrowRight, Loader2 } from 'lucide-react';

interface AuthViewProps {
  onAuthSuccess: (token: string) => void; // פונקציה שתקרא במקרה של הצלחה ותעביר אותנו ל-Dashboard
}

const AuthView: React.FC<AuthViewProps> = ({ onAuthSuccess }) => {
  // ניהול מצבי המסך
  const [isLogin, setIsLogin] = useState<boolean>(true);
  const [isLoading, setIsLoading] = useState<boolean>(false);
  const [errorMsg, setErrorMsg] = useState<string | null>(null);

  // ניהול שדות הטופס
  const [formData, setFormData] = useState({
    username: '',
    email: '',
    password: '',
  });

  // עדכון שדות הטופס
  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setFormData({ ...formData, [e.target.name]: e.target.value });
  };

  // שליחת הטופס לשרת (סימולציה)
  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setIsLoading(true);
    setErrorMsg(null);

    try {
      // כאן תכנס קריאת ה-Axios האמיתית ל-Backend שלך:
      // const response = await axios.post(`/api/auth/${isLogin ? 'login' : 'signup'}`, formData);

      // סימולציית השהייה של רשת (למחוק בפרודקשן)
      await new Promise((resolve) => setTimeout(resolve, 1500));

      // סימולציה של הצלחה - מעביר טוקן דמה
      const mockJwtToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...";
      onAuthSuccess(mockJwtToken);

    } catch (err) {
      setErrorMsg(isLogin ? "שגיאה בפרטי ההתחברות. נסה שוב." : "אירעה שגיאה בעת יצירת החשבון.");
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-slate-950 flex items-center justify-center p-4 font-sans text-slate-200">
      <div className="max-w-md w-full bg-slate-900 border border-slate-800 rounded-2xl shadow-2xl overflow-hidden relative">

        {/* פס דקורטיבי עליון (אווירת סייבר) */}
        <div className="h-1 w-full bg-gradient-to-r from-cyan-500 to-blue-600"></div>

        <div className="p-8">
          {/* כותרת ולוגו */}
          <div className="text-center mb-8">
            <div className="mx-auto bg-slate-800/50 w-16 h-16 rounded-full flex items-center justify-center border border-slate-700 mb-4 shadow-[0_0_15px_rgba(6,182,212,0.2)]">
              <ShieldAlert className="w-8 h-8 text-cyan-400" />
            </div>
            <h1 className="text-2xl font-bold tracking-wider text-white">WireSherlock</h1>
            <p className="text-slate-400 text-sm mt-1">Advanced Network Forensics</p>
          </div>

          {/* מנגנון החלפה: Login / Sign Up */}
          <div className="flex p-1 bg-slate-950 rounded-lg mb-8 border border-slate-800">
            <button
              type="button"
              onClick={() => setIsLogin(true)}
              className={`flex-1 py-2 text-sm font-medium rounded-md transition-all duration-200 ${
                isLogin
                  ? 'bg-slate-800 text-cyan-400 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
              }`}
            >
              Login
            </button>
            <button
              type="button"
              onClick={() => setIsLogin(false)}
              className={`flex-1 py-2 text-sm font-medium rounded-md transition-all duration-200 ${
                !isLogin
                  ? 'bg-slate-800 text-cyan-400 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900'
              }`}
            >
              Sign Up
            </button>
          </div>

          {/* הודעת שגיאה במידה ויש */}
          {errorMsg && (
            <div className="mb-6 p-3 bg-red-500/10 border border-red-500/50 rounded-lg text-red-400 text-sm text-center">
              {errorMsg}
            </div>
          )}

          {/* טופס התחברות / הרשמה */}
          <form onSubmit={handleSubmit} className="space-y-5">

            {/* שדה שם משתמש (מופיע רק בהרשמה) */}
            {!isLogin && (
              <div className="space-y-1 relative">
                <label className="text-xs font-semibold text-slate-400 uppercase tracking-wider">Username</label>
                <div className="relative">
                  <User className="absolute left-3 top-1/2 -translate-y-1/2 w-5 h-5 text-slate-500" />
                  <input
                    type="text"
                    name="username"
                    value={formData.username}
                    onChange={handleChange}
                    required={!isLogin}
                    className="w-full bg-slate-950 border border-slate-700 text-white rounded-lg pl-10 pr-4 py-2.5 focus:outline-none focus:border-cyan-500 focus:ring-1 focus:ring-cyan-500 transition-colors"
                    placeholder="investigator_01"
                  />
                </div>
              </div>
            )}

            {/* שדה אימייל (משותף) */}
            <div className="space-y-1 relative">
              <label className="text-xs font-semibold text-slate-400 uppercase tracking-wider">Email Address</label>
              <div className="relative">
                <Mail className="absolute left-3 top-1/2 -translate-y-1/2 w-5 h-5 text-slate-500" />
                <input
                  type="email"
                  name="email"
                  value={formData.email}
                  onChange={handleChange}
                  required
                  className="w-full bg-slate-950 border border-slate-700 text-white rounded-lg pl-10 pr-4 py-2.5 focus:outline-none focus:border-cyan-500 focus:ring-1 focus:ring-cyan-500 transition-colors"
                  placeholder="analyst@domain.com"
                />
              </div>
            </div>

            {/* שדה סיסמה (משותף) */}
            <div className="space-y-1 relative">
              <label className="text-xs font-semibold text-slate-400 uppercase tracking-wider">Password</label>
              <div className="relative">
                <Lock className="absolute left-3 top-1/2 -translate-y-1/2 w-5 h-5 text-slate-500" />
                <input
                  type="password"
                  name="password"
                  value={formData.password}
                  onChange={handleChange}
                  required
                  className="w-full bg-slate-950 border border-slate-700 text-white rounded-lg pl-10 pr-4 py-2.5 focus:outline-none focus:border-cyan-500 focus:ring-1 focus:ring-cyan-500 transition-colors"
                  placeholder="••••••••"
                />
              </div>
            </div>

            {/* כפתור שליחה */}
            <button
              type="submit"
              disabled={isLoading}
              className="w-full mt-6 bg-cyan-600 hover:bg-cyan-500 text-white font-semibold py-3 px-4 rounded-lg transition-all duration-200 flex items-center justify-center space-x-2 disabled:opacity-70 disabled:cursor-not-allowed group"
            >
              {isLoading ? (
                <>
                  <Loader2 className="w-5 h-5 animate-spin" />
                  <span>Authenticating...</span>
                </>
              ) : (
                <>
                  <span>{isLogin ? 'Access System' : 'Initialize Account'}</span>
                  <ArrowRight className="w-4 h-4 opacity-70 group-hover:translate-x-1 transition-transform" />
                </>
              )}
            </button>

          </form>
        </div>
      </div>
    </div>
  );
};

export default AuthView;