export interface Env {
  NEXUS_DB: D1Database;
  PRODUCT_NAME: string;
  ALLOWED_ORIGIN: string;
  LATEST_VERSION: string;
  LATEST_PACKAGE_URL: string;
  LATEST_PACKAGE_SHA256: string;
  LATEST_PACKAGE_SIZE: string;
  MINIMUM_WINDOWS_BUILD: string;
  SETUP_DOWNLOAD_URL: string;
  STRIPE_TEST_SECRET_KEY: string;
  STRIPE_LIVE_SECRET_KEY: string;
  STRIPE_TEST_WEBHOOK_SECRET: string;
  STRIPE_LIVE_WEBHOOK_SECRET: string;
  STRIPE_TEST_PRICE_30_DAY: string;
  STRIPE_TEST_PRICE_LIFETIME: string;
  STRIPE_LIVE_PRICE_30_DAY: string;
  STRIPE_LIVE_PRICE_LIFETIME: string;
  STRIPE_SECRET_KEY?: string;
  STRIPE_WEBHOOK_SECRET?: string;
  STRIPE_PRICE_30_DAY?: string;
  STRIPE_PRICE_LIFETIME?: string;
  LICENSE_HASH_PEPPER: string;
  LICENSE_ENCRYPTION_KEY_B64: string;
  LICENSE_SIGNING_PRIVATE_JWK: string;
}

export type LicenseType = "30_day" | "lifetime";
export type StripeMode = "test" | "live";

export interface StripeCheckoutSession {
  id: string;
  livemode?: boolean;
  payment_status: string;
  payment_intent?: string | null;
  customer_email?: string | null;
  customer_details?: { email?: string | null } | null;
  metadata?: Record<string, string> | null;
  line_items?: {
    data?: Array<{
      price?: { id?: string | null } | null;
      quantity?: number | null;
    }>;
  } | null;
}

export interface LicenseRow {
  id: number;
  key_hash: string;
  key_prefix: string;
  key_ciphertext: string;
  key_iv: string;
  license_type: LicenseType;
  status: string;
  stripe_session_id: string;
  stripe_payment_intent_id: string | null;
  stripe_customer_email: string;
  stripe_price_id: string;
  device_hash: string | null;
  created_at: string;
  activated_at: string | null;
  expires_at: string | null;
  last_seen_at: string | null;
  key_revealed_at: string | null;
  revoke_reason: string | null;
}
