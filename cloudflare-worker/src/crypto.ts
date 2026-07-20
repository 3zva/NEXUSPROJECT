import { base64Decode, base64Encode, base64UrlEncode, bufferSource, bytesToHex, utf8 } from "./encoding";
import type { Env } from "./types";

const KEY_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

export function normalizeLicenseKey(value: string): string {
  return value.trim().toUpperCase().replace(/\s+/g, "");
}

export function normalizeEmail(value: string): string {
  return value.trim().toLowerCase();
}

export function generateLicenseKey(): string {
  const random = crypto.getRandomValues(new Uint8Array(20));
  let body = "";
  for (const byte of random) body += KEY_ALPHABET[byte % KEY_ALPHABET.length];
  return `NXS-${body.slice(0, 5)}-${body.slice(5, 10)}-${body.slice(10, 15)}-${body.slice(15, 20)}`;
}

async function importHmac(secret: string): Promise<CryptoKey> {
  return crypto.subtle.importKey("raw", bufferSource(utf8(secret)), { name: "HMAC", hash: "SHA-256" }, false, ["sign"]);
}

export async function hmacHex(secret: string, value: string): Promise<string> {
  const key = await importHmac(secret);
  return bytesToHex(new Uint8Array(await crypto.subtle.sign("HMAC", key, bufferSource(utf8(value)))));
}

export async function hashLicenseKey(env: Env, key: string): Promise<string> {
  return hmacHex(env.LICENSE_HASH_PEPPER, `license:${normalizeLicenseKey(key)}`);
}

export async function hashDevice(env: Env, device: string): Promise<string> {
  return hmacHex(env.LICENSE_HASH_PEPPER, `device:${device.trim().toLowerCase()}`);
}

async function importAesKey(env: Env, usage: KeyUsage[]): Promise<CryptoKey> {
  const bytes = base64Decode(env.LICENSE_ENCRYPTION_KEY_B64);
  if (bytes.length !== 32) throw new Error("LICENSE_ENCRYPTION_KEY_B64 must decode to 32 bytes.");
  return crypto.subtle.importKey("raw", bufferSource(bytes), "AES-GCM", false, usage);
}

export async function encryptLicenseKey(env: Env, plaintext: string): Promise<{ ciphertext: string; iv: string }> {
  const iv = crypto.getRandomValues(new Uint8Array(12));
  const key = await importAesKey(env, ["encrypt"]);
  const encrypted = await crypto.subtle.encrypt({ name: "AES-GCM", iv: bufferSource(iv) }, key, bufferSource(utf8(plaintext)));
  return { ciphertext: base64Encode(new Uint8Array(encrypted)), iv: base64Encode(iv) };
}

export async function decryptLicenseKey(env: Env, ciphertext: string, iv: string): Promise<string> {
  const key = await importAesKey(env, ["decrypt"]);
  const decrypted = await crypto.subtle.decrypt({ name: "AES-GCM", iv: bufferSource(base64Decode(iv)) }, key, bufferSource(base64Decode(ciphertext)));
  return new TextDecoder().decode(decrypted);
}

export async function signObject(env: Env, payload: Record<string, unknown>): Promise<string> {
  const header = { alg: "ES256", typ: "NEXUS" };
  const headerPart = base64UrlEncode(utf8(JSON.stringify(header)));
  const payloadPart = base64UrlEncode(utf8(JSON.stringify(payload)));
  const privateJwk = JSON.parse(env.LICENSE_SIGNING_PRIVATE_JWK) as JsonWebKey;
  const key = await crypto.subtle.importKey("jwk", privateJwk, { name: "ECDSA", namedCurve: "P-256" }, false, ["sign"]);
  const signature = await crypto.subtle.sign({ name: "ECDSA", hash: "SHA-256" }, key, bufferSource(utf8(`${headerPart}.${payloadPart}`)));
  return `${headerPart}.${payloadPart}.${base64UrlEncode(new Uint8Array(signature))}`;
}
