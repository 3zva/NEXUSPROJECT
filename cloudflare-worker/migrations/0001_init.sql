PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS licenses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    key_hash TEXT NOT NULL UNIQUE,
    key_prefix TEXT NOT NULL,
    key_ciphertext TEXT NOT NULL,
    key_iv TEXT NOT NULL,
    license_type TEXT NOT NULL CHECK (license_type IN ('30_day', 'lifetime')),
    status TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'revoked', 'refunded', 'disputed', 'disabled')),
    stripe_session_id TEXT NOT NULL UNIQUE,
    stripe_payment_intent_id TEXT,
    stripe_customer_email TEXT NOT NULL,
    stripe_price_id TEXT NOT NULL,
    device_hash TEXT,
    created_at TEXT NOT NULL,
    activated_at TEXT,
    expires_at TEXT,
    last_seen_at TEXT,
    key_revealed_at TEXT,
    revoke_reason TEXT
);

CREATE INDEX IF NOT EXISTS idx_licenses_payment_intent
    ON licenses(stripe_payment_intent_id);
CREATE INDEX IF NOT EXISTS idx_licenses_email
    ON licenses(stripe_customer_email);
CREATE INDEX IF NOT EXISTS idx_licenses_status
    ON licenses(status);

CREATE TABLE IF NOT EXISTS stripe_events (
    event_id TEXT PRIMARY KEY,
    event_type TEXT NOT NULL,
    received_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS license_audit (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    license_id INTEGER,
    action TEXT NOT NULL,
    occurred_at TEXT NOT NULL,
    detail TEXT,
    FOREIGN KEY (license_id) REFERENCES licenses(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS idx_audit_license
    ON license_audit(license_id, occurred_at);
