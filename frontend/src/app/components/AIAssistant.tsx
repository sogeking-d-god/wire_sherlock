import { useState } from 'react';
import { Send, Sparkles } from 'lucide-react';

interface Message {
  role: 'user' | 'assistant';
  content: string;
  timestamp: Date;
}

export function AIAssistant() {
  const [messages, setMessages] = useState<Message[]>([
    {
      role: 'assistant',
      content: 'Analyzing PCAP segment 192_0046...\n\nI detected a sudden increase in TCP SYN packets originating from 192.168.1.5 directed at port 443 on 10.0.2.14.\n\nSuspect IP: 192.168.1.5→10.0.2.14:443\nFlags: [S]\nRate: IP 192.168.1.5:54352 > 10.0.2.14:443\nFlags: [S]\n...[6 more lines omitted]...\n\nConclusion: This pattern is highly indicative of a SYN flood attempt. The target node (10.0.2.14) is currently dropping ~15% of packets as seen in the metrics.',
      timestamp: new Date(Date.now() - 120000),
    },
    {
      role: 'user',
      content: 'Analyze the spike in traffic from 192.168.1.5 to 10.0.2.14 over the last 5 minutes.',
      timestamp: new Date(Date.now() - 180000),
    },
  ]);
  const [input, setInput] = useState('');

  const handleSend = () => {
    if (!input.trim()) return;

    const newMessage: Message = {
      role: 'user',
      content: input,
      timestamp: new Date(),
    };

    setMessages([...messages, newMessage]);
    setInput('');

    // Simulate AI response
    setTimeout(() => {
      const aiMessage: Message = {
        role: 'assistant',
        content: 'Analyzing network patterns...',
        timestamp: new Date(),
      };
      setMessages(prev => [...prev, aiMessage]);
    }, 1000);
  };

  return (
    <div className="h-full bg-[#0f172a] flex flex-col border-l border-[#31394d]">
      {/* Header */}
      <div className="h-12 bg-[#060e20] border-b border-[#31394d] flex items-center justify-between px-4 shrink-0">
        <div className="flex items-center gap-2">
          <div className="w-2 h-2 bg-[#10b981] rounded-full animate-pulse" />
          <Sparkles className="w-4 h-4 text-[#00a3ff]" />
          <span className="font-bold text-[#dae2fd] text-[11px] tracking-[0.88px]">
            NET_VISOR_AI_ASSISTANT
          </span>
        </div>
        <span className="text-[9px] text-[#64748b]">Q2</span>
      </div>

      {/* Session Info */}
      <div className="px-4 py-2 border-b border-[#31394d]/50 shrink-0">
        <div className="text-[#64748b] text-[11px] font-mono">
          SESSION_START // 10:45:16 UTC
        </div>
      </div>

      {/* Messages */}
      <div className="flex-1 overflow-y-auto px-4 py-4 space-y-4">
        {messages.slice().reverse().map((msg, idx) => (
          <div key={idx} className={`flex flex-col gap-2 ${msg.role === 'user' ? 'items-end' : 'items-start'}`}>
            <div className={`max-w-[90%] rounded-lg p-3 ${
              msg.role === 'user'
                ? 'bg-[#00a3ff] text-[#0b1326]'
                : 'bg-[#171f33] text-[#dae2fd] border border-[#31394d]'
            }`}>
              <div className="text-[12px] leading-relaxed whitespace-pre-wrap">
                {msg.content}
              </div>
            </div>
            {msg.role === 'assistant' && (
              <div className="flex gap-2 px-2">
                <button className="text-[#64748b] hover:text-[#00a3ff] text-[9px] px-2 py-1 border border-[#31394d] rounded">
                  BLOCK_IP(192.168.1.5)
                </button>
                <button className="text-[#64748b] hover:text-[#00a3ff] text-[9px] px-2 py-1 border border-[#31394d] rounded">
                  VIEW_RAW_PACKETS
                </button>
              </div>
            )}
          </div>
        ))}
      </div>

      {/* Input */}
      <div className="border-t border-[#31394d] p-4 shrink-0">
        <div className="flex gap-2">
          <input
            type="text"
            value={input}
            onChange={(e) => setInput(e.target.value)}
            onKeyDown={(e) => e.key === 'Enter' && handleSend()}
            placeholder="Ask about the pcap data..."
            className="flex-1 bg-[#171f33] border border-[#31394d] rounded px-3 py-2 text-[13px] text-[#dae2fd] placeholder:text-[#3f4852] outline-none focus:border-[#00a3ff]"
          />
          <button
            onClick={handleSend}
            className="w-10 h-10 bg-[#00a3ff] rounded flex items-center justify-center hover:bg-[#0090e0] transition-colors"
          >
            <Send className="w-4 h-4 text-[#0b1326]" />
          </button>
        </div>
        <div className="mt-2 text-[9px] text-[#64748b]">
          AI Content. Current timezone: UTC
        </div>
      </div>
    </div>
  );
}
