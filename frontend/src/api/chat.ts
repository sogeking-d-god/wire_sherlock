/**
 * SSE client for POST /api/chat.
 *
 * We can't use the browser's EventSource because it only supports GET, and
 * doesn't let us set Content-Type or send a JSON body. Instead, we POST with
 * fetch + credentials: "include" (to send the auth cookie) and parse the
 * text/event-stream frames manually from a ReadableStream.
 *
 * Frame format produced by the backend (backend/routers/chat_router.py):
 *   event: <name>\n
 *   data: <json>\n
 *   \n
 *
 * The agent emits three event names: "token", "tool_call", "done".
 */

import { API_BASE_URL } from "../config";

export type ToolCallStatus = "started" | "completed" | "failed";

export interface ToolCallEventData {
  name: string;
  arguments: Record<string, unknown>;
  status: ToolCallStatus;
  message?: string;
}

export type ChatEvent =
  | { event: "token";     data: { delta: string } }
  | { event: "tool_call"; data: ToolCallEventData }
  | { event: "done";      data: { reason: string; detail?: string } };

export interface StreamChatOptions {
  signal?: AbortSignal;
  maxToolHops?: number;
}

export async function* streamChat(
  prompt: string,
  options: StreamChatOptions = {},
): AsyncGenerator<ChatEvent, void, void> {
  const res = await fetch(`${API_BASE_URL}/api/chat`, {
    method: "POST",
    credentials: "include",
    headers: {
      "Content-Type": "application/json",
      Accept: "text/event-stream",
    },
    body: JSON.stringify({
      prompt,
      ...(options.maxToolHops ? { max_tool_hops: options.maxToolHops } : {}),
    }),
    signal: options.signal,
  });

  if (!res.ok) {
    let detail = `chat failed: HTTP ${res.status}`;
    try {
      const body = await res.json();
      if (body && typeof body === "object" && "detail" in body) {
        detail = String((body as { detail: unknown }).detail);
      }
    } catch { /* ignore */ }
    throw new Error(detail);
  }
  if (!res.body) throw new Error("chat: response had no body");

  const reader = res.body.getReader();
  const decoder = new TextDecoder();
  let buffer = "";

  try {
    while (true) {
      const { value, done } = await reader.read();
      if (done) break;
      buffer += decoder.decode(value, { stream: true });

      // Normalize CRLF -> LF so the "\n\n" frame separator works regardless of
      // anything in the request path that touched line endings.
      if (buffer.indexOf("\r\n") !== -1) {
        buffer = buffer.replace(/\r\n/g, "\n");
      }

      // SSE frames are separated by a blank line ("\n\n").
      let sepIdx: number;
      while ((sepIdx = buffer.indexOf("\n\n")) >= 0) {
        const frame = buffer.slice(0, sepIdx);
        buffer = buffer.slice(sepIdx + 2);
        const parsed = parseFrame(frame);
        if (parsed) {
          // Temporary diagnostic — remove once the stream is verified stable.
          // eslint-disable-next-line no-console
          console.log("[streamChat] event:", parsed.event, parsed.data);
          yield parsed;
        }
      }
    }
  } finally {
    try { reader.releaseLock(); } catch { /* ignore */ }
  }
}

function parseFrame(frame: string): ChatEvent | null {
  let eventName = "";
  let dataLine = "";
  for (const rawLine of frame.split("\n")) {
    const line = rawLine.replace(/\r$/, "");
    if (!line || line.startsWith(":")) continue; // SSE comment / heartbeat
    if (line.startsWith("event:")) {
      eventName = line.slice(6).trim();
    } else if (line.startsWith("data:")) {
      // Per the SSE spec, multiple data: lines concatenate with "\n", but our
      // server emits a single data line per frame — handle both gracefully.
      dataLine += (dataLine ? "\n" : "") + line.slice(5).trim();
    }
  }
  if (!eventName || !dataLine) return null;
  try {
    const data = JSON.parse(dataLine);
    return { event: eventName, data } as ChatEvent;
  } catch {
    console.warn("[streamChat] dropped malformed frame:", frame);
    return null;
  }
}
