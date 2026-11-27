# Snipz security verification plan

This plan is for the current Cloudflare Worker scaffold. It is not a penetration test.

## Current enforced boundary

- The Worker fails closed while ALLOWED_ORIGIN is unset or a placeholder.
- Browser origins must match the configured HTTPS origin exactly.
- Only GET /api/health is implemented.
- Every other path returns 404; every non-GET method returns 405.
- The Worker does not accept user input for database writes and does not expose D1 or R2.
- Responses use restrictive API/browser headers and generic errors.

## Required negative tests before enabling any data route

Run these against a non-production environment after configuring a test frontend origin:

1. No ALLOWED_ORIGIN: health request returns 503.
2. Placeholder or non-HTTPS ALLOWED_ORIGIN: health request returns 503.
3. Wrong Origin: request returns 403 and has no permissive CORS header.
4. Correct Origin: health route returns only service health, never credentials or binding details.
5. POST, PUT, PATCH and DELETE: return 405.
6. Unknown paths and alternate path casing: return 404.
7. OPTIONS from an unapproved origin: return 403.
8. OPTIONS from the approved origin: only GET is advertised.
9. Simulate a D1 failure: response is generic 503 with no stack trace or database details.
10. Verify the deployed response includes the configured security headers and no-store behavior.

## Before implementing accounts, chat, snaps, or uploads

- Select one identity provider and validate signed access tokens server-side. Never trust a user ID, role, or ownership claim from request JSON.
- Add deny-by-default authorization tests for every resource and relationship.
- Add server-side block checks to every discovery, profile, notification, comment, chat and media route.
- Add abuse limits backed by a shared store/provider controls; in-memory Worker counters are not sufficient.
- Keep media private; use short-lived object-specific upload/download authorization and inspect actual file bytes.
- Do not store private message or snap content until retention, deletion, access rules, and encryption requirements are designed.
- Add moderation queues, human review workflows, moderator MFA and audit logging.
- Test account deletion, data export, backups and restoration.
- Have an independent security review before a public launch.

## Important

No tests or deployment are performed by adding this plan. A green health check is not a security certification. Snipz must not be presented as fully secure or opened to real users until the controls above are implemented and verified.
