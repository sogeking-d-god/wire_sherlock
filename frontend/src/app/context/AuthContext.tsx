import { createContext, useContext, ReactNode } from 'react';

interface AuthContextType {
  /** Display name of the logged-in user (falls back to the user_id UUID). */
  username: string | null;
  userId: string | null;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);

export function AuthProvider({
  children,
  username,
  userId,
}: {
  children: ReactNode;
  username: string | null;
  userId: string | null;
}) {
  return (
    <AuthContext.Provider value={{ username, userId }}>
      {children}
    </AuthContext.Provider>
  );
}

export function useAuth(): AuthContextType {
  const ctx = useContext(AuthContext);
  if (ctx === undefined) {
    throw new Error('useAuth must be used within an AuthProvider');
  }
  return ctx;
}
