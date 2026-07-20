# Deployment checklist

## Cloudflare

- [ ] Cloudflare account remains on Workers Free.
- [ ] D1 database created and ID inserted into wrangler.jsonc.
- [ ] Migration applied remotely.
- [ ] Secret generator run once in a secure local folder.
- [ ] Private JWK, pepper, AES key, Stripe secret, and webhook secret added through Wrangler secrets.
- [ ] Only public JWK copied into C++ projects.
- [ ] Worker deployed.
- [ ] `/health` works.

## Stripe test mode

- [ ] Real test Price ID for 30-day inserted.
- [ ] Real test Price ID for lifetime inserted.
- [ ] Metadata exactly matches each license type.
- [ ] Redirect uses `{CHECKOUT_SESSION_ID}`.
- [ ] Webhook endpoint configured.
- [ ] Duplicate webhook test creates one key only.
- [ ] Wrong email cannot reveal key.
- [ ] Refund and dispute revoke the key.

## C++ application

- [ ] Worker URL set.
- [ ] Public signing key embedded.
- [ ] First activation online.
- [ ] DPAPI storage works.
- [ ] Token signature, nonce, device, and times verified.
- [ ] 72-hour offline grace tested.
- [ ] No private secrets in binary strings.

## Setup wizard / release

- [ ] Release x64 build created with no distributed PDB.
- [ ] Runtime dependencies packaged.
- [ ] Application ZIP hash and size calculated.
- [ ] Setup EXE and application ZIP uploaded to GitHub Release.
- [ ] Worker release variables updated.
- [ ] Manifest signature verification tested.
- [ ] Corrupted ZIP rejected.
- [ ] Interrupted update rolls back.
- [ ] Website Download button uses Worker `/download` URL.

## Production

- [ ] Switch Stripe environment and Worker secrets from test to live.
- [ ] Replace test Price IDs with live Price IDs.
- [ ] Re-run a real low-value end-to-end purchase and refund test.
- [ ] Keep private keys and recovery copies offline.
