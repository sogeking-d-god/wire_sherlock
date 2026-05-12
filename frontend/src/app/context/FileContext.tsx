import { createContext, useContext, useState, useEffect, ReactNode } from 'react';

interface FileContextType {
  activeFileId: string | null;
  activeFileName: string | null;
  setActiveFile: (fileId: string, fileName: string) => void;
  clearActiveFile: () => void;
}

const FileContext = createContext<FileContextType | undefined>(undefined);

export function FileProvider({ children }: { children: ReactNode }) {
  const [activeFileId, setActiveFileId] = useState<string | null>(null);
  const [activeFileName, setActiveFileName] = useState<string | null>(null);

  // Load from sessionStorage on mount
  useEffect(() => {
    const storedFileId = sessionStorage.getItem('selectedFileId');
    const storedFileName = sessionStorage.getItem('selectedFileName');
    if (storedFileId) {
      setActiveFileId(storedFileId);
      setActiveFileName(storedFileName);
    }
  }, []);

  const setActiveFile = (fileId: string, fileName: string) => {
    setActiveFileId(fileId);
    setActiveFileName(fileName);
    sessionStorage.setItem('selectedFileId', fileId);
    sessionStorage.setItem('selectedFileName', fileName);
  };

  const clearActiveFile = () => {
    setActiveFileId(null);
    setActiveFileName(null);
    sessionStorage.removeItem('selectedFileId');
    sessionStorage.removeItem('selectedFileName');
  };

  return (
    <FileContext.Provider
      value={{ activeFileId, activeFileName, setActiveFile, clearActiveFile }}
    >
      {children}
    </FileContext.Provider>
  );
}

export function useFileContext() {
  const context = useContext(FileContext);
  if (context === undefined) {
    throw new Error('useFileContext must be used within a FileProvider');
  }
  return context;
}
