/**
 * Centralized HTTP client for the WireSherlock backend.
 *
 * Every authenticated call MUST go through `apiFetch` so that we never forget
 * `credentials: "include"` — the backend sets the JWT in an HttpOnly cookie
 * (see backend/routers/auth_router.py::_set_auth_cookie), which the browser
 * only attaches to cross-origin fetches when credentials mode is "include".
 */

import { API_BASE_URL } from "../config";

export class AuthError extends Error {
  constructor(message = "not authenticated") {
    super(message);
    this.name = "AuthError";
  }
}

export class ApiError extends Error {
  status: number;
  body: unknown;
  constructor(status: number, body: unknown, message?: string) {
    super(message ?? `HTTP ${status}`);
    this.name = "ApiError";
    this.status = status;
    this.body = body;
  }
}

export async function apiFetch(path: string, init: RequestInit = {}): Promise<Response> {
  const url = path.startsWith("http") ? path : `${API_BASE_URL}${path}`;
  const headers = new Headers(init.headers ?? {});
  // Only force JSON content-type for string/JSON-ish bodies. For FormData, Blob,
  // URLSearchParams, etc., the browser sets the correct Content-Type (with the
  // multipart boundary for FormData), and overriding it would break the upload.
  const body = init.body;
  const isStructuredBody =
    body instanceof FormData ||
    body instanceof Blob ||
    body instanceof URLSearchParams ||
    body instanceof ArrayBuffer;
  if (body && !isStructuredBody && !headers.has("Content-Type")) {
    headers.set("Content-Type", "application/json");
  }

  const res = await fetch(url, {
    ...init,
    credentials: "include",
    headers,
  });

  if (res.status === 401) {
    throw new AuthError();
  }
  return res;
}

export async function apiJson<T>(path: string, init?: RequestInit): Promise<T> {
  const res = await apiFetch(path, init);
  if (!res.ok) {
    let body: unknown = null;
    try { body = await res.json(); } catch { /* ignore */ }
    const detail = (body && typeof body === "object" && "detail" in body)
      ? String((body as { detail: unknown }).detail)
      : `HTTP ${res.status}`;
    throw new ApiError(res.status, body, detail);
  }
  return res.json() as Promise<T>;
}
