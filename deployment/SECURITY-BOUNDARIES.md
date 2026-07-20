# Security boundaries

## Public and expected to be downloadable

- Website HTML/CSS/JavaScript
- Stripe Payment Links
- Worker public API URL
- Setup wizard executable
- NEXUS application executable
- Public signing key
- Release manifest and package hash

## Private and server-side only

- Stripe secret key
- Stripe webhook secret
- license hash pepper
- license AES encryption key
- ECDSA private signing key
- D1 records

## Reality check

A determined person can inspect and patch a local executable. The design therefore keeps entitlement truth on the server and uses short-lived signed responses. Obfuscation is only an additional delay, not the security boundary.
