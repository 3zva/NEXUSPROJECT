import { errorResponse, html, json, readJson } from "./http";
import { activateOrValidate, fulfillCheckoutSession, revealOrder, revokeByPaymentIntent } from "./licenses";
import { retrieveChargePaymentIntent, verifyStripeWebhook } from "./stripe";
import { signObject } from "./crypto";
import { successPage } from "./success-page";
import type { Env } from "./types";

interface StripeEvent { id: string; type: string; data: { object: Record<string, unknown> }; }

async function handleWebhook(request: Request, env: Env): Promise<Response> {
  const raw = await request.text();
  await verifyStripeWebhook(raw, request.headers.get("Stripe-Signature"), env.STRIPE_WEBHOOK_SECRET);
  const event = JSON.parse(raw) as StripeEvent;
  const claimed = await env.NEXUS_DB.prepare(`
    INSERT OR IGNORE INTO stripe_events (event_id, event_type, received_at)
    VALUES (?1, ?2, ?3)
  `).bind(event.id, event.type, new Date().toISOString()).run();
  if ((claimed.meta.changes ?? 0) === 0) return json({ received: true, duplicate: true });

  try {
    if (event.type === "checkout.session.completed" || event.type === "checkout.session.async_payment_succeeded") {
      const sessionId = String(event.data.object.id ?? "");
      await fulfillCheckoutSession(env, sessionId);
    } else if (event.type === "charge.refunded") {
      const paymentIntent = typeof event.data.object.payment_intent === "string" ? event.data.object.payment_intent : "";
      if (paymentIntent) await revokeByPaymentIntent(env, paymentIntent, "refunded", event.type);
    } else if (event.type === "charge.dispute.created") {
      const chargeId = typeof event.data.object.charge === "string" ? event.data.object.charge : "";
      if (chargeId) {
        const paymentIntent = await retrieveChargePaymentIntent(env, chargeId);
        if (paymentIntent) await revokeByPaymentIntent(env, paymentIntent, "disputed", event.type);
      }
    }
  } catch (error) {
    await env.NEXUS_DB.prepare("DELETE FROM stripe_events WHERE event_id = ?1").bind(event.id).run();
    throw error;
  }

  return json({ received: true });
}

async function releaseManifest(env: Env): Promise<Response> {
  const issuedAt = new Date().toISOString();
  const payload = {
    product: env.PRODUCT_NAME,
    channel: "stable",
    version: env.LATEST_VERSION,
    package_url: env.LATEST_PACKAGE_URL,
    package_sha256: env.LATEST_PACKAGE_SHA256.toLowerCase(),
    package_size: Number(env.LATEST_PACKAGE_SIZE),
    minimum_windows_build: Number(env.MINIMUM_WINDOWS_BUILD),
    published_at: issuedAt,
    manifest_id: crypto.randomUUID()
  };
  return json({ ok: true, manifest: payload, signature: await signObject(env, payload) }, 200, env);
}

export default {
  async fetch(request: Request, env: Env): Promise<Response> {
    const url = new URL(request.url);
    try {
      if (request.method === "GET" && url.pathname === "/health") return json({ ok: true, service: "nexus-license-api", time: new Date().toISOString() });
      if (request.method === "POST" && url.pathname === "/stripe/webhook") return handleWebhook(request, env);
      if (request.method === "GET" && url.pathname === "/success") return html(successPage(url.searchParams.get("session_id") ?? ""));
      if (request.method === "POST" && url.pathname === "/v1/order/reveal") {
        const body = await readJson<{ session_id?: string; email?: string }>(request);
        const revealed = await revealOrder(env, body.session_id ?? "", body.email ?? "");
        return json({ ok: true, license_key: revealed.key, license_type: revealed.type }, 200, env);
      }
      if (request.method === "POST" && url.pathname === "/v1/license/activate") return json(await activateOrValidate(env, await readJson<Record<string, unknown>>(request), true), 200, env);
      if (request.method === "POST" && url.pathname === "/v1/license/validate") return json(await activateOrValidate(env, await readJson<Record<string, unknown>>(request), false), 200, env);
      if (request.method === "GET" && url.pathname === "/v1/releases/latest") return releaseManifest(env);
      if (request.method === "GET" && url.pathname === "/download") return Response.redirect(env.SETUP_DOWNLOAD_URL, 302);
      return json({ ok: false, code: "not_found", message: "Route not found." }, 404, env);
    } catch (error) {
      return errorResponse(error, env);
    }
  }
} satisfies ExportedHandler<Env>;
