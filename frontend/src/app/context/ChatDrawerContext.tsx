import { createContext, useCallback, useContext, useState, ReactNode } from 'react';

interface ChatDrawerContextType {
  isOpen: boolean;
  open: () => void;
  close: () => void;
  toggle: () => void;
}

const ChatDrawerContext = createContext<ChatDrawerContextType | undefined>(undefined);

export function ChatDrawerProvider({
  children,
  initialOpen = true,
}: {
  children: ReactNode;
  initialOpen?: boolean;
}) {
  const [isOpen, setIsOpen] = useState<boolean>(initialOpen);

  const open = useCallback(() => setIsOpen(true), []);
  const close = useCallback(() => setIsOpen(false), []);
  const toggle = useCallback(() => setIsOpen(v => !v), []);

  return (
    <ChatDrawerContext.Provider value={{ isOpen, open, close, toggle }}>
      {children}
    </ChatDrawerContext.Provider>
  );
}

export function useChatDrawer(): ChatDrawerContextType {
  const ctx = useContext(ChatDrawerContext);
  if (ctx === undefined) {
    throw new Error('useChatDrawer must be used within a ChatDrawerProvider');
  }
  return ctx;
}
