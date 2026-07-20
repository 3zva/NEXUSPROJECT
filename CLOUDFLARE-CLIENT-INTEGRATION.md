# Native C++ client changes

The starter directory contains the previous WinHTTP/DPAPI client. Codex must update it for the Worker API.

## Endpoints

- POST `/v1/license/activate`
- POST `/v1/license/validate`

## Request JSON

```json
{
  "license_key": "NXS-...",
  "device_id": "locally-hashed-stable-device-id",
  "client_version": "1.0.0",
  "nonce": "cryptographically-random-request-nonce"
}
```

## Required response verification

Do not trust the visible JSON fields by themselves. Parse and verify `validation_token` using the embedded public ECDSA P-256 JWK. Verify:

- signature
- issuer
- token version
- nonce equals the request nonce
- device hash equals this device
- issued-at and expiration
- 30-day license expiry
- no impossible trusted-time rollback

Store the key and last verified token with DPAPI. Keep user configuration separate from program files so setup/update/uninstall does not accidentally remove profiles.
