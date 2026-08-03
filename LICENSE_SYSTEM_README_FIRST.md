# NEXUS Cloudflare License + Setup Wizard Starter

This package replaces the always-on private server with:

- Cloudflare Worker: private server-side API
- Cloudflare D1: license and payment records
- Stripe Payment Links: hosted checkout
- GitHub Releases: `https://github.com/3zva/NEXUSPROJECT/releases` hosts the public standalone NEXUS EXE
- Native C++ NEXUS client: activation and validation only
- Native C++ setup wizard: downloads and verifies the latest signed release

## Security boundary

License generation occurs only inside the Cloudflare Worker after a verified Stripe payment. The website and C++ application never contain the generator, Stripe secret, webhook secret, database credentials, license pepper, encryption key, or signing private key.

## Start here

1. Run `setup-on-desktop.ps1`.
2. Open the created Desktop folder in Codex.
3. Paste `CODEX_PROMPT.txt` into Codex.
4. Follow `deployment/DEPLOYMENT-CHECKLIST.md`.

The starter Worker is intentionally designed to be reviewed and completed by Codex before production deployment. Use Stripe test mode first.
