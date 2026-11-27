# Snipz security baseline

## Current state — not production-ready

The Worker intentionally exposes only a health check. It has no login, user data, upload, chat, snap, follow, comment, report-processing or moderation API. All write methods return 405. The frontend safety screen is a local demonstration and does not enforce blocks or submit reports.

Do not connect real users or upload personal media until the controls below are implemented, reviewed and tested. A successful health check is not a security audit.

## Additional API baseline

The API Worker also sends restrictive browser/API headers (including no-sniff, no-referrer, frame denial, a deny-all CSP, no-store, and cross-origin isolation hints). HSTS is limited to the Worker host rather than using `includeSubDomains` or `preload`, which could unintentionally affect unrelated subdomains. See [the negative security test plan](./cloudflare/SECURITY-TEST-PLAN.md).

## Added database design proposal (not deployed)

The D1 schema now includes proposed tables for user blocks, friend requests, private-by-default account settings, safety reports covering users/videos/comments/messages, and moderator actions. The schema file has **not** been applied to D1. These tables do not protect anything until authenticated API routes enforce them on every request. Authentication must come from a maintained identity provider; never trust a user ID, role, or permission sent by the browser.

The Worker now compares the incoming Origin header to the configured HTTPS origin exactly, rather than normalizing it. This is an additional browser-origin check, not authentication: non-browser clients can omit Origin, so every future private route still needs token verification and authorization.

## Required before any real-user pilot

### Identity and sessions
- Use a maintained authentication provider; never implement password storage yourself.
- Require verified email or other age-appropriate account verification; do not collect identity documents casually.
- Enforce server-side session validation on every private API route; rotate/revoke sessions on logout, password change and account recovery.
- Add MFA for staff/moderators. Never put service-role keys, signing secrets or admin credentials in browser code or Git.
- Add login, signup, reset and verification rate limits and abuse monitoring.

### Authorization and privacy
- Deny by default. Check ownership and relationship permissions on every read and write, not just in the UI.
- Private profiles and friend-only content must be private in database queries and media delivery.
- Keep precise location, contact-book uploads and public activity status off by default.
- Implement block relationships on the server and make blocks apply to discovery, chats, snaps, comments and notifications.
- Use short-lived, narrowly scoped media access URLs; never expose bucket credentials or permanent public object URLs for private media.

### Media uploads
- Upload only through short-lived server-issued authorization.
- Enforce byte limits, quotas, allow-listed formats, actual file-signature checks, random object names and per-account rate limits.
- Reject executable/HTML/SVG content unless there is a reviewed sanitization pipeline. Re-encode images and transcode video in an isolated service.
- Scan and moderate media before public distribution. Strip unnecessary EXIF metadata, especially location.
- Store private media in a private bucket. D1 is metadata storage, not video storage.

### Community safety
- Implement server-persisted reports for accounts, videos, comments and messages.
- Provide trained human review, escalation paths, evidence retention rules and appeals before public launch.
- Add mute/block, anti-spam limits, message requests and controls against unsolicited contact.
- Build moderator roles with least privilege and immutable audit events. Never trust a role sent by the browser.
- Provide clear emergency guidance; do not promise that Snipz is monitored continuously.

### Data protection and operations
- Collect the minimum data needed. Define retention periods and deletion workflows for accounts, content, backups and logs.
- Keep secrets in provider-managed secret storage, not source files or frontend configuration.
- Configure provider-level rate limiting/WAF, alerts, backups and restore drills.
- Use separate development and production environments.
- Add automated tests for IDOR/access control, XSS, CSRF where cookie auth is used, CORS, upload abuse, rate limits and deletion.
- Run dependency and secret scanning; arrange an independent security review before a public launch.

## Deployment notes

- Set `ALLOWED_ORIGIN` to the exact HTTPS origin of the deployed Snipz frontend. The Worker fails closed while it is a placeholder.
- The Worker currently supports only `GET /api/health`. Do not add write routes until authentication and authorization middleware and tests are in place.
- Keep the D1 database and R2 bucket private. Never publish credentials.
- No deployment was performed by this change.

## Important
This checklist is an engineering starting point, not legal advice, a penetration test, certification or a guarantee against breaches or legal claims. Because Snipz may be used by minors and may process personal data, involve a parent/guardian and qualified privacy/security professionals before any real-user test or public launch.
