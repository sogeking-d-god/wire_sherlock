import { useState, useEffect, useRef } from 'react';
import { Upload, FileText, Play, Clock, CheckCircle2, Loader2, HardDrive, RefreshCw } from 'lucide-react';
import { useNavigate } from 'react-router';
import { Button } from './ui/button';
import { Progress } from './ui/progress';
import { useFileContext } from '../context/FileContext';
import { toast } from 'sonner';

import { API_ENDPOINTS } from '../../config';


interface PcapFile {
  file_id: string;
  filename: string;
  upload_date: string;
  status: 'ready' | 'analyzing' | 'processed';
  file_size?: number;
}

export function FileManagement() {
  const [files, setFiles] = useState<PcapFile[]>([]);
  const [isDragging, setIsDragging] = useState(false);
  const [uploadProgress, setUploadProgress] = useState<number | null>(null);
  const [isUploading, setIsUploading] = useState(false);
  const [isLoading, setIsLoading] = useState(true);
  const [analyzingFileId, setAnalyzingFileId] = useState<string | null>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);
  const navigate = useNavigate();
  const { setActiveFile } = useFileContext();

  // Initial load
  useEffect(() => {
    fetchFiles();
  }, []);

  const fetchFiles = async () => {
    try {
      setIsLoading(true);
      const response = await fetch(API_ENDPOINTS.PCAP.FILES);

      if (response.ok) {
        const data = await response.json();

        const rawFiles = Array.isArray(data) ? data : (data.files || []);

        const formattedFiles: PcapFile[] = rawFiles.map((file: any) => ({
          file_id: file.id,
          filename: file.name,
          upload_date: file.upload_date || new Date().toLocaleDateString(),
          status: file.status || 'ready',
          file_size: file.file_size
        }));

        setFiles(formattedFiles);
      } else {
        throw new Error('Failed to fetch file list');
      }
    } catch (error) {
      console.error('Failed to fetch files:', error);
    } finally {
      setIsLoading(false);
    }
  };


  const handleDragOver = (e: React.DragEvent) => {
    e.preventDefault();
    setIsDragging(true);
  };

  const handleDragLeave = () => {
    setIsDragging(false);
  };

  const handleDrop = async (e: React.DragEvent) => {
    e.preventDefault();
    setIsDragging(false);

    const droppedFiles = Array.from(e.dataTransfer.files);
    const pcapFile = droppedFiles.find(f =>
      f.name.endsWith('.pcap') || f.name.endsWith('.pcapng')
    );

    if (pcapFile) {
      await uploadFile(pcapFile);
    }
  };

  const handleFileSelect = (e: React.ChangeEvent<HTMLInputElement>) => {
    const selectedFiles = e.target.files;
    if (selectedFiles && selectedFiles[0]) {
      uploadFile(selectedFiles[0]);
    }
  };

  const uploadFile = async (file: File) => {
    setIsUploading(true);
    setUploadProgress(0);

    try {
      const formData = new FormData();
      formData.append('file', file);

      const progressInterval = setInterval(() => {
        setUploadProgress(prev => {
          if (prev === null || prev >= 90) return 90;
          return prev + 10;
        });
      }, 200);

      // CHANGE: Using centralized config for upload
      const response = await fetch(API_ENDPOINTS.PCAP.UPLOAD, {
        method: 'POST',
        body: formData,
      });

      clearInterval(progressInterval);
      setUploadProgress(100);

      if (response.ok) {
        setTimeout(() => {
          setUploadProgress(null);
          setIsUploading(false);
          fetchFiles(); // Refreshing the list
          toast.success('File uploaded successfully!');
        }, 500);
      } else {
        throw new Error('Upload failed');
      }
    } catch (error) {
      console.error('Upload error:', error);
      setUploadProgress(null);
      setIsUploading(false);
      toast.error('Failed to upload file');
    }
  };

  const handleAnalyze = async (fileId: string, fileName: string) => {
    try {
      setAnalyzingFileId(fileId);

      // CHANGE: Using dynamic helper from config
      const url = API_ENDPOINTS.PCAP.SELECT(fileId);

      const response = await fetch(url, {
        method: 'POST',
      });

      if (response.ok) {
        // Update global context
        setActiveFile(fileId, fileName);

        toast.success('Session initialized successfully!', {
          description: `Analyzing ${fileName}`,
        });

        setTimeout(() => {
          navigate('/dashboard');
        }, 500);
      } else {
        throw new Error('Failed to select file');
      }
    } catch (error) {
      console.error('Failed to select file:', error);
      toast.error('Selection failed', {
        description: 'Check if the backend is running.',
      });
      setAnalyzingFileId(null);
    }
  };

  const getStatusBadge = (status: string) => {
    const s = status?.toLowerCase() || '';

    switch (s) {
      case 'ready':
        return (
          <div className="flex items-center gap-2 text-[#3b82f6]">
            <CheckCircle2 className="w-4 h-4" />
            <span className="text-[12px] font-medium uppercase tracking-wide">Ready</span>
          </div>
        );
      case 'analyzing':
        return (
          <div className="flex items-center gap-2 text-[#f59e0b]">
            <Loader2 className="w-4 h-4 animate-spin" />
            <span className="text-[12px] font-medium uppercase tracking-wide">Analyzing</span>
          </div>
        );
      case 'processed':
        return (
          <div className="flex items-center gap-2 text-[#10b981]">
            <CheckCircle2 className="w-4 h-4" />
            <span className="text-[12px] font-medium uppercase tracking-wide">Processed</span>
          </div>
        );
      default:
        return <span className="text-[12px] text-[#64748b]">Unknown</span>;
    }
  };

  const formatDate = (dateString: string) => {
    if (!dateString) return 'N/A';
    const date = new Date(dateString);

    // Check if date is valid before formatting
    if (isNaN(date.getTime())) return 'Invalid Date';

    return date.toLocaleDateString('en-US', {
      month: 'short',
      day: 'numeric',
      year: 'numeric',
      hour: '2-digit',
      minute: '2-digit'
    });
  };

  const formatFileSize = (bytes?: number) => {
    if (!bytes) return 'N/A';
    const mb = bytes / (1024 * 1024);
    return `${mb.toFixed(2)} MB`;
  };

  return (
    <div className="flex-1 bg-[#0b1326] overflow-auto">
      <div className="max-w-7xl mx-auto p-8">
        {/* Header */}
        <div className="mb-8">
          <h1 className="text-[28px] font-black text-[#00a3ff] tracking-[1.4px] mb-2">
            PCAP ARCHIVE
          </h1>
          <p className="text-[14px] text-[#64748b]">
            Upload and manage packet capture files for network analysis
          </p>
        </div>

        {/* Upload Zone */}
        <div
          className={`border-2 border-dashed rounded-lg p-12 mb-8 transition-all ${
            isDragging
              ? 'border-[#00a3ff] bg-[#00a3ff]/5'
              : 'border-[#31394d] hover:border-[#00a3ff]/50'
          }`}
          onDragOver={handleDragOver}
          onDragLeave={handleDragLeave}
          onDrop={handleDrop}
        >
          <div className="flex flex-col items-center justify-center gap-4">
            <div className="w-16 h-16 rounded-full bg-[#171f33] flex items-center justify-center">
              <Upload className="w-8 h-8 text-[#00a3ff]" />
            </div>
            <div className="text-center">
              <p className="text-[16px] font-bold text-[#e2e8f0] mb-2">
                Drag & Drop PCAP Files
              </p>
              <p className="text-[13px] text-[#64748b] mb-4">
                or click below to browse
              </p>
              <Button
                onClick={() => fileInputRef.current?.click()}
                disabled={isUploading}
                className="bg-[#00a3ff] hover:bg-[#0090e0] text-white"
              >
                Browse Files
              </Button>
              <input
                ref={fileInputRef}
                type="file"
                accept=".pcap,.pcapng"
                className="hidden"
                onChange={handleFileSelect}
              />
            </div>
            {uploadProgress !== null && (
              <div className="w-full max-w-md mt-4">
                <Progress value={uploadProgress} className="h-2" />
                <p className="text-[12px] text-[#64748b] mt-2 text-center">
                  Uploading: {uploadProgress}%
                </p>
              </div>
            )}
          </div>
        </div>

        {/* Demo File Card */}
        <div className="bg-gradient-to-r from-[#00a3ff]/10 to-[#6366f1]/10 border border-[#00a3ff]/30 rounded-lg p-6 mb-6">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-4">
              <div className="w-12 h-12 rounded-lg bg-[#00a3ff]/20 flex items-center justify-center">
                <HardDrive className="w-6 h-6 text-[#00a3ff]" />
              </div>
              <div>
                <div className="flex items-center gap-2 mb-1">
                  <h3 className="text-[16px] font-bold text-[#00a3ff]">
                    SYSTEM DEMO FILE
                  </h3>
                  <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-[#6366f1]/20 text-[#a5b4fc] uppercase tracking-wide">
                    Demo
                  </span>
                </div>
                <p className="text-[13px] text-[#64748b]">
                  Pre-loaded sample PCAP for testing and demonstration
                </p>
              </div>
            </div>
            <Button
              onClick={() => handleAnalyze('demo-pcap', 'System Demo File')}
              disabled={analyzingFileId === 'demo-pcap'}
              className="bg-[#00a3ff] hover:bg-[#0090e0] text-white gap-2"
            >
              {analyzingFileId === 'demo-pcap' ? (
                <>
                  <Loader2 className="w-4 h-4 animate-spin" />
                  Starting Session...
                </>
              ) : (
                <>
                  <Play className="w-4 h-4" />
                  Analyze
                </>
              )}
            </Button>
          </div>
        </div>

        {/* Files Table */}
        <div className="bg-[#060e20] border border-[#31394d] rounded-lg overflow-hidden">
          <div className="px-6 py-4 border-b border-[#31394d]">
            <h2 className="text-[14px] font-bold text-[#e2e8f0] uppercase tracking-[1.4px]">
              Uploaded Files
            </h2>
            <button
              onClick={fetchFiles}
              disabled={isLoading}
              className="p-2 hover:bg-[#171f33] rounded-md transition-all group"
              title="Refresh file list"
            >
              <RefreshCw className={`w-4 h-4 text-[#64748b] group-hover:text-[#00a3ff] ${isLoading ? 'animate-spin' : ''}`} />
            </button>
          </div>

          {isLoading ? (
            <div className="flex items-center justify-center py-12">
              <Loader2 className="w-8 h-8 text-[#00a3ff] animate-spin" />
            </div>
          ) : files.length === 0 ? (
            <div className="flex flex-col items-center justify-center py-12 text-center">
              <FileText className="w-12 h-12 text-[#31394d] mb-4" />
              <p className="text-[14px] text-[#64748b]">
                No files uploaded yet. Drop a PCAP file above to get started.
              </p>
            </div>
          ) : (
            <div className="overflow-x-auto">
              <table className="w-full">
                <thead>
                  <tr className="bg-[#171f33] border-b border-[#31394d]">
                    <th className="px-6 py-3 text-left text-[11px] font-bold text-[#64748b] uppercase tracking-[1.1px]">
                      File Name
                    </th>
                    <th className="px-6 py-3 text-left text-[11px] font-bold text-[#64748b] uppercase tracking-[1.1px]">
                      Type
                    </th>
                    <th className="px-6 py-3 text-left text-[11px] font-bold text-[#64748b] uppercase tracking-[1.1px]">
                      Upload Date
                    </th>
                    <th className="px-6 py-3 text-left text-[11px] font-bold text-[#64748b] uppercase tracking-[1.1px]">
                      Size
                    </th>
                    <th className="px-6 py-3 text-left text-[11px] font-bold text-[#64748b] uppercase tracking-[1.1px]">
                      Status
                    </th>
                    <th className="px-6 py-3 text-right text-[11px] font-bold text-[#64748b] uppercase tracking-[1.1px]">
                      Actions
                    </th>
                  </tr>
                </thead>
                <tbody>
                  {files.map((file) => (
                    <tr
                      key={file.file_id}
                      className="border-b border-[#31394d]/50 hover:bg-[#171f33]/30 transition-colors"
                    >
                      <td className="px-6 py-4">
                        <div className="flex items-center gap-3">
                          <FileText className="w-5 h-5 text-[#00a3ff]" />
                          <span className="text-[13px] text-[#e2e8f0] font-medium">
                            {file.filename}
                          </span>
                        </div>
                      </td>
                      <td className="px-6 py-4">
                        <span className="px-2 py-1 rounded text-[10px] font-bold bg-[#10b981]/20 text-[#34d399] uppercase tracking-wide">
                          Uploaded
                        </span>
                      </td>
                      <td className="px-6 py-4">
                        <div className="flex items-center gap-2 text-[#94a3b8]">
                          <Clock className="w-4 h-4" />
                          <span className="text-[12px]">
                            {formatDate(file.upload_date)}
                          </span>
                        </div>
                      </td>
                      <td className="px-6 py-4">
                        <span className="text-[12px] text-[#94a3b8]">
                          {formatFileSize(file.file_size)}
                        </span>
                      </td>
                      <td className="px-6 py-4">
                        {getStatusBadge(file.status)}
                      </td>
                      <td className="px-6 py-4 text-right">
                        <Button
                          onClick={() => handleAnalyze(file.file_id, file.filename)}
                          disabled={analyzingFileId === file.file_id}
                          size="sm"
                          className="bg-[#00a3ff] hover:bg-[#0090e0] text-white gap-2"
                        >
                          {analyzingFileId === file.file_id ? (
                            <>
                              <Loader2 className="w-3 h-3 animate-spin" />
                              Starting...
                            </>
                          ) : (
                            <>
                              <Play className="w-3 h-3" />
                              Analyze
                            </>
                          )}
                        </Button>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
