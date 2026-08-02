import { decryptLicenseKey, encryptLicenseKey, generateLicenseKey, hashDevice, hashLicenseKey, normalizeEmail, signObject } from "./crypto";
import { retrieveCheckoutSession } from "./stripe";
import type { Env, LicenseRow, LicenseType, StripeCheckoutSession, StripeMode } from "./types";

function nowIso(): string { return new Date().toISOString(); }

function stripeModeForSession(session: StripeCheckoutSession): StripeMode {
  if (session.id.startsWith("cs_test_") || session.livemode === false) return "test";
  if (session.id.startsWith("cs_live_") || session.livemode === true) return "live";
  throw new Error("Unable to determine Stripe mode for Checkout Session.");
}

function priceIdsForMode(env: Env, mode: StripeMode): { day30: string; lifetime: string } {
  const prices = mode === "test"
    ? {
      day30: env.STRIPE_TEST_PRICE_30_DAY || env.STRIPE_PRICE_30_DAY || "",
      lifetime: env.STRIPE_TEST_PRICE_LIFETIME || env.STRIPE_PRICE_LIFETIME || ""
    }
    : {
      day30: env.STRIPE_LIVE_PRICE_30_DAY || "",
      lifetime: env.STRIPE_LIVE_PRICE_LIFETIME || ""
    };
  if (!prices.day30 || !prices.lifetime) throw new Error(`Stripe ${mode} Price IDs are not configured.`);
  return prices;
}

function supportedPurchase(env: Env, session: StripeCheckoutSession): { type: LicenseType; priceId: string; email: string } {
  if (session.payment_status !== "paid") throw new Error("Checkout Session is not paid.");
  const items = session.line_items?.data ?? [];
  if (items.length !== 1 || (items[0]?.quantity ?? 1) !== 1) throw new Error("Checkout Session must contain exactly one supported license.");
  const priceId = items[0]?.price?.id ?? "";
  const prices = priceIdsForMode(env, stripeModeForSession(session));
  let type: LicenseType;
  if (priceId === prices.day30) type = "30_day";
  else if (priceId === prices.lifetime) type = "lifetime";
  else throw new Error("Checkout Session contains an unsupported Price ID.");
  if (session.metadata?.license_type !== type) throw new Error("Stripe metadata does not match the purchased Price ID.");
  const email = normalizeEmail(session.customer_details?.email ?? session.customer_email ?? "");
  if (!email || !email.includes("@")) throw new Error("Checkout Session does not include a valid customer email.");
  return { type, priceId, email };
}

export async function fulfillCheckoutSession(env: Env, sessionOrId: StripeCheckoutSession | string): Promise<LicenseRow> {
  const session = typeof sessionOrId === "string" ? await retrieveCheckoutSession(env, sessionOrId) : sessionOrId;
  const existing = await env.NEXUS_DB.prepare("SELECT * FROM licenses WHERE stripe_session_id = ?1").bind(session.id).first<LicenseRow>();
  if (existing) return existing;

  const purchase = supportedPurchase(env, session);
  for (let attempt = 0; attempt < 5; attempt += 1) {
    const rawKey = generateLicenseKey();
    const keyHash = await hashLicenseKey(env, rawKey);
    const encrypted = await encryptLicenseKey(env, rawKey);
    const createdAt = nowIso();
    try {
      await env.NEXUS_DB.prepare(`
        INSERT INTO licenses (
          key_hash, key_prefix, key_ciphertext, key_iv, license_type, status,
          stripe_session_id, stripe_payment_intent_id, stripe_customer_email,
          stripe_price_id, created_at
        ) VALUES (?1, ?2, ?3, ?4, ?5, 'active', ?6, ?7, ?8, ?9, ?10)
      `).bind(
        keyHash, rawKey.slice(0, 9), encrypted.ciphertext, encrypted.iv,
        purchase.type, session.id, session.payment_intent ?? null,
        purchase.email, purchase.priceId, createdAt
      ).run();
      const inserted = await env.NEXUS_DB.prepare("SELECT * FROM licenses WHERE stripe_session_id = ?1").bind(session.id).first<LicenseRow>();
      if (!inserted) throw new Error("License insert did not return a record.");
      await audit(env, inserted.id, "created", `stripe_session=${session.id}`);
      return inserted;
    } catch (error) {
      const raced = await env.NEXUS_DB.prepare("SELECT * FROM licenses WHERE stripe_session_id = ?1").bind(session.id).first<LicenseRow>();
      if (raced) return raced;
      if (attempt === 4) throw error;
    }
  }
  throw new Error("Unable to generate a unique license key.");
}

export async function revealOrder(env: Env, sessionId: string, emailInput: string): Promise<{ key: string; type: LicenseType }> {
  const session = await retrieveCheckoutSession(env, sessionId);
  const purchase = supportedPurchase(env, session);
  if (normalizeEmail(emailInput) !== purchase.email) throw new Error("The email does not match the completed checkout.");
  const license = await fulfillCheckoutSession(env, session);
  const key = await decryptLicenseKey(env, license.key_ciphertext, license.key_iv);
  await env.NEXUS_DB.prepare("UPDATE licenses SET key_revealed_at = COALESCE(key_revealed_at, ?1) WHERE id = ?2").bind(nowIso(), license.id).run();
  await audit(env, license.id, "revealed", "success_page");
  return { key, type: license.license_type };
}

export async function activateOrValidate(env: Env, body: Record<string, unknown>, activate: boolean): Promise<Record<string, unknown>> {
  const licenseKey = typeof body.license_key === "string" ? body.license_key : "";
  const deviceId = typeof body.device_id === "string" ? body.device_id : "";
  const clientVersion = typeof body.client_version === "string" ? body.client_version.slice(0, 64) : "unknown";
  const nonce = typeof body.nonce === "string" ? body.nonce.slice(0, 128) : "";
  if (licenseKey.length < 10 || licenseKey.length > 128) throw new Error("Invalid license key format.");
  if (deviceId.length < 16 || deviceId.length > 256) throw new Error("Invalid device identifier.");
  if (nonce.length < 16) throw new Error("A request nonce is required.");

  const keyHash = await hashLicenseKey(env, licenseKey);
  const storedDeviceHash = await hashDevice(env, deviceId);
  let license = await env.NEXUS_DB.prepare("SELECT * FROM licenses WHERE key_hash = ?1").bind(keyHash).first<LicenseRow>();
  if (!license) throw new Error("License not found.");
  if (license.status !== "active") throw new Error(`License is ${license.status}.`);

  const now = new Date();
  if (!license.activated_at) {
    if (!activate) throw new Error("License must be activated first.");
    const activatedAt = now.toISOString();
    const expiresAt = license.license_type === "30_day" ? new Date(now.getTime() + 30 * 24 * 60 * 60 * 1000).toISOString() : null;
    const result = await env.NEXUS_DB.prepare(`
      UPDATE licenses SET activated_at = ?1, expires_at = ?2, device_hash = ?3, last_seen_at = ?1
      WHERE id = ?4 AND activated_at IS NULL AND device_hash IS NULL AND status = 'active'
    `).bind(activatedAt, expiresAt, storedDeviceHash, license.id).run();
    if ((result.meta.changes ?? 0) !== 1) throw new Error("License activation changed concurrently; retry.");
    license = await env.NEXUS_DB.prepare("SELECT * FROM licenses WHERE id = ?1").bind(license.id).first<LicenseRow>();
    if (!license) throw new Error("License disappeared after activation.");
    await audit(env, license.id, "activated", `client=${clientVersion}`);
  } else {
    if (license.device_hash !== storedDeviceHash) throw new Error("License is bound to another device.");
    await env.NEXUS_DB.prepare("UPDATE licenses SET last_seen_at = ?1 WHERE id = ?2").bind(now.toISOString(), license.id).run();
  }

  if (license.expires_at && Date.parse(license.expires_at) <= now.getTime()) throw new Error("License has expired.");

  const issuedAt = Math.floor(now.getTime() / 1000);
  const tokenExpires = issuedAt + 24 * 60 * 60;
  const token = await signObject(env, {
    v: 1,
    iss: "nexus-license-api",
    sub: String(license.id),
    typ: license.license_type,
    dev: storedDeviceHash,
    iat: issuedAt,
    exp: tokenExpires,
    server_time: now.toISOString(),
    license_expires_at: license.expires_at,
    nonce,
    jti: crypto.randomUUID()
  });

  return {
    ok: true,
    valid: true,
    status: license.status,
    license_type: license.license_type,
    activated_at: license.activated_at,
    expires_at: license.expires_at,
    server_time: now.toISOString(),
    validation_token: token,
    token_expires_at: new Date(tokenExpires * 1000).toISOString()
  };
}

export async function revokeByPaymentIntent(env: Env, paymentIntent: string, status: "refunded" | "disputed", reason: string): Promise<void> {
  const rows = await env.NEXUS_DB.prepare("SELECT id FROM licenses WHERE stripe_payment_intent_id = ?1").bind(paymentIntent).all<{ id: number }>();
  await env.NEXUS_DB.prepare("UPDATE licenses SET status = ?1, revoke_reason = ?2 WHERE stripe_payment_intent_id = ?3").bind(status, reason, paymentIntent).run();
  for (const row of rows.results ?? []) await audit(env, row.id, status, reason);
}

export async function audit(env: Env, licenseId: number | null, action: string, detail: string): Promise<void> {
  await env.NEXUS_DB.prepare("INSERT INTO license_audit (license_id, action, occurred_at, detail) VALUES (?1, ?2, ?3, ?4)")
    .bind(licenseId, action, nowIso(), detail.slice(0, 500)).run();
}
