import type { Env } from "./types";

export function json(data: unknown, status = 200, env?: Env): Response {
  const headers = new Headers({
    "content-type": "application/json; charset=utf-8",
    "cache-control": "no-store, max-age=0",
    "x-content-type-options": "nosniff",
    "referrer-policy": "no-referrer"
  });
  if (env?.ALLOWED_ORIGIN) headers.set("access-control-allow-origin", env.ALLOWED_ORIGIN);
  return new Response(JSON.stringify(data), { status, headers });
}

export function html(body: string, status = 200): Response {
  return new Response(body, {
    status,
    headers: {
      "content-type": "text/html; charset=utf-8",
      "cache-control": "no-store, max-age=0",
      "content-security-policy": "default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline'; connect-src 'self'; img-src 'self' data:; base-uri 'none'; form-action 'self'; frame-ancestors 'none'",
      "x-content-type-options": "nosniff",
      "referrer-policy": "no-referrer",
      "x-frame-options": "DENY"
    }
  });
}

export async function readJson<T>(request: Request, maxBytes = 16_384): Promise<T> {
  const length = Number(request.headers.get("content-length") ?? "0");
  if (Number.isFinite(length) && length > maxBytes) throw new Error("Request body is too large.");
  const text = await request.text();
  if (text.length > maxBytes) throw new Error("Request body is too large.");
  return JSON.parse(text) as T;
}

export function errorResponse(error: unknown, env?: Env): Response {
  const message = error instanceof Error ? error.message : "Unexpected error.";
  console.error(error);
  return json({ ok: false, code: "request_failed", message }, 400, env);
}
