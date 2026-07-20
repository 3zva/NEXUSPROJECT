import { bufferSource, constantTimeEqual, hexToBytes, utf8 } from "./encoding";
import type { Env, StripeCheckoutSession } from "./types";

function parseStripeSignature(header: string): { timestamp: number; signatures: string[] } {
  let timestamp = 0;
  const signatures: string[] = [];
  for (const part of header.split(",")) {
    const [name, value] = part.split("=", 2);
    if (name === "t") timestamp = Number(value);
    if (name === "v1" && value) signatures.push(value);
  }
  if (!timestamp || signatures.length === 0) throw new Error("Invalid Stripe signature header.");
  return { timestamp, signatures };
}

export async function verifyStripeWebhook(rawBody: string, signatureHeader: string | null, secret: string): Promise<void> {
  if (!signatureHeader) throw new Error("Missing Stripe-Signature header.");
  const { timestamp, signatures } = parseStripeSignature(signatureHeader);
  if (Math.abs(Math.floor(Date.now() / 1000) - timestamp) > 300) throw new Error("Stripe webhook timestamp is outside the allowed tolerance.");
  const key = await crypto.subtle.importKey("raw", bufferSource(utf8(secret)), { name: "HMAC", hash: "SHA-256" }, false, ["sign"]);
  const expected = new Uint8Array(await crypto.subtle.sign("HMAC", key, bufferSource(utf8(`${timestamp}.${rawBody}`))));
  const valid = signatures.some((candidate) => {
    try { return constantTimeEqual(expected, hexToBytes(candidate)); } catch { return false; }
  });
  if (!valid) throw new Error("Stripe webhook signature verification failed.");
}

async function stripeGet<T>(env: Env, path: string, params?: URLSearchParams): Promise<T> {
  const url = new URL(`https://api.stripe.com/v1${path}`);
  if (params) url.search = params.toString();
  const response = await fetch(url, {
    headers: { Authorization: `Bearer ${env.STRIPE_SECRET_KEY}` }
  });
  const data = await response.json() as T & { error?: { message?: string } };
  if (!response.ok) throw new Error(data.error?.message ?? `Stripe API request failed (${response.status}).`);
  return data;
}

export async function retrieveCheckoutSession(env: Env, sessionId: string): Promise<StripeCheckoutSession> {
  if (!/^cs_(test_|live_)?[A-Za-z0-9_]+$/.test(sessionId)) throw new Error("Invalid Checkout Session ID.");
  const params = new URLSearchParams();
  params.append("expand[]", "line_items.data.price");
  return stripeGet<StripeCheckoutSession>(env, `/checkout/sessions/${encodeURIComponent(sessionId)}`, params);
}

export async function retrieveChargePaymentIntent(env: Env, chargeId: string): Promise<string | null> {
  const charge = await stripeGet<{ payment_intent?: string | null }>(env, `/charges/${encodeURIComponent(chargeId)}`);
  return charge.payment_intent ?? null;
}
