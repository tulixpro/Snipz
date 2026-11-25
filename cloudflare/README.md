# Snipz on Cloudflare (migration plan)

This folder documents the proposed Supabase-free hosting architecture.

## Important: this is a scaffold, not a live migration

The current app still uses Supabase Auth, Database and Storage. Do not remove the existing setup until the Cloudflare backend has been deployed and tested. No cloud account or storage has been created by committing these files.

## Proposed services

- Cloudflare Pages: static website hosting.
- Cloudflare Workers: API and server-side authorization.
- Cloudflare D1: managed SQL database.
- Cloudflare R2: video object storage.
- Cloudflare Turnstile: bot/spam protection.

Cloudflare manages the database service; you do not need to create or publish a database-server password. Account login and deployment authorization are still required. Store any secrets as Worker secrets, never in GitHub files or browser JavaScript.

## Free-tier reality

Free plans have quotas, limits and product-specific restrictions. Video storage and especially video delivery can exceed free allowances quickly. Millions of videos or millions of views cannot be promised free. Add upload size limits, rate limits, per-account quotas, abuse controls, alerts and a hard operational budget before opening uploads publicly. Check Cloudflare's current pricing and terms before launch.

## Admin and verification policy

- Reserved admin handle: `Snips`.
- Never ship an admin password, admin token, or admin role in client-side code.
- Provision the first admin only through a trusted server-side setup process after deployment. Store the admin identity in a Worker secret or protected D1 record; require strong authentication and preferably MFA.
- Admin moderation actions must be performed by the Worker after checking the authenticated user's server-side role. Hiding an admin button in the browser is not authorization.
- Verified badge policy requested: accounts with at least 1,000,000 followers qualify automatically. Counts must be calculated from server/database records, not a value supplied by the browser. Admin may separately grant/revoke verification with an audit log.
- Admin moderation should support review queues, remove/restore videos, suspend accounts, and record a reason, actor and timestamp. Keep user reports and an appeal path.

## Safe migration order

1. Create a Cloudflare account and enable Pages, Workers, D1, R2 and Turnstile.
2. Deploy the API with private bindings and secrets.
3. Create D1 tables and test authorization policies.
4. Add authenticated upload URLs so video bytes go directly to R2 without exposing account secrets.
5. Migrate account sign-in and existing profile/video metadata.
6. Test owner-only edits/deletes, report submission, admin moderation, rate limits and backups.
7. Switch the website to the new API only after tests pass; then remove Supabase references.

Never paste passwords, API tokens, or private keys into chat or commit them to this repository.
