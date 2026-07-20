# Cloudflare Worker setup

## Create the project

1. Install Node.js LTS.
2. Run `npm install`.
3. Run `npx wrangler login`.
4. Create D1:
   `npx wrangler d1 create nexus-license-db`
5. Copy the returned database ID into `wrangler.jsonc`.
6. Apply migrations:
   `npm run db:remote`
7. Generate local secrets:
   `npm run secrets:generate`
8. Put every secret into Cloudflare with `npx wrangler secret put NAME`.
9. Set both real Stripe Price IDs and release URLs in `wrangler.jsonc`.
10. Deploy with `npm run deploy`.

## Required secrets

- STRIPE_SECRET_KEY
- STRIPE_WEBHOOK_SECRET
- LICENSE_HASH_PEPPER
- LICENSE_ENCRYPTION_KEY_B64
- LICENSE_SIGNING_PRIVATE_JWK

Never place secret values in `wrangler.jsonc`, Git, the website, or C++ source.

## Stripe webhook

Add the deployed endpoint:

`https://YOUR-WORKER.workers.dev/stripe/webhook`

Subscribe to:

- checkout.session.completed
- checkout.session.async_payment_succeeded
- charge.refunded
- charge.dispute.created

Use Stripe test mode until all tests pass.
