# WordPress + Stripe integration

The free WordPress site is only the storefront.

## Payment links

Create two separate Stripe Payment Links:

- 30-day: $7 one-time; metadata `license_type` = `30_day`
- Lifetime: $20 one-time; metadata `license_type` = `lifetime`

Set each Payment Link's after-payment redirect to:

`https://YOUR-WORKER-DOMAIN/success?session_id={CHECKOUT_SESSION_ID}`

Use the matching public Payment Link on each website purchase button.

## Download button

Point the website Download button to:

`https://YOUR-WORKER-DOMAIN/download`

The Worker redirects to the current public `NEXUS-VERSION.exe` from `https://github.com/3zva/NEXUSPROJECT/releases`, allowing releases to change without editing WordPress.

## Never put on WordPress

- Stripe secret key
- webhook secret
- license generator
- D1 data
- hash pepper
- AES key
- private signing key
- raw license database
