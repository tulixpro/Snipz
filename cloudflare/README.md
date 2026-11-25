# Snipz Cloudflare backend

This is an initial, deliberately limited deployment scaffold — **not a live server**.
The current website still uses Supabase. Do not switch the frontend until authentication,
authorization, uploads, moderation and migration tests are complete.

## Components

- Pages: static website.
- Worker: API entry point (src/index.js).
- D1: managed SQL database (../schema.sql is the initial schema proposal).
- R2: video object storage.
- Turnstile: bot protection on signup and other public forms.

## What the Worker currently does

- Exposes GET /api/health to test the Worker and D1 binding.
- Restricts browser CORS to the exact ALLOWED_ORIGIN.
- Adds basic browser security headers.
- Returns generic errors rather than leaking infrastructure details.
- Does not expose D1 or R2 directly to the browser.
- Does not yet implement sign-in, upload, feed mutations, admin actions or moderation.

A green health check is not proof that the social app backend is production-ready.

## Deploy preparation

1. Create a Cloudflare account and a Pages project for this repository.
2. Install Wrangler locally and authenticate with npx wrangler login.
3. Create D1 database snipz-db and R2 bucket snipz-videos.
4. Replace the D1 database ID and Pages origin in wrangler.toml.
5. Apply the reviewed SQL schema to D1; do not apply it to a production database without a backup.
6. Run npx wrangler dev and verify /api/health.
7. Deploy with npx wrangler deploy, then verify the deployed health endpoint.

Cloudflare account access and deployment approval are required. Never paste account passwords,
API tokens, database credentials or private keys into chat or GitHub.

## Security work required before public launch

- Use a maintained identity provider and verify every access token server-side on every protected route.
- Derive the user ID and role from the verified token; never trust client-supplied user IDs or role flags.
- Implement per-user and per-IP rate limits, Turnstile, upload quotas and request-size limits.
- Issue short-lived, object-specific upload URLs only after authorization; validate file type and size.
- Keep uploaded media private until moderation/scanning is complete; never accept arbitrary object keys from clients.
- Add report queues, moderator authorization, audit logs, account suspension, appeals and admin MFA.
- Calculate follower counts and the 1,000,000-follower verification rule on the server.
- Add backup/restore tests, monitoring, alerts, privacy controls, account deletion and data export.
- Configure Cloudflare WAF/rate-limiting rules and review current pricing/quotas.

The reserved brand/admin handle is Snipz. No admin account or credentials are created by this scaffold.
