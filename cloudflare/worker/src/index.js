const SECURITY_HEADERS = {
  "X-Content-Type-Options": "nosniff",
  "Referrer-Policy": "no-referrer",
  "X-Frame-Options": "DENY",
  "Permissions-Policy": "camera=(), microphone=(), geolocation=(), payment=(), usb=()",
  "Cross-Origin-Resource-Policy": "same-site",
  "Cross-Origin-Opener-Policy": "same-origin",
  "Cache-Control": "no-store",
  "Content-Security-Policy": "default-src 'none'; frame-ancestors 'none'; base-uri 'none'",
  "Strict-Transport-Security": "max-age=31536000; includeSubDomains; preload",
};

function json(data, status, origin) {
  return new Response(JSON.stringify(data), {
    status,
    headers: {
      ...SECURITY_HEADERS,
      "Content-Type": "application/json; charset=utf-8",
      ...(origin ? { "Access-Control-Allow-Origin": origin, "Vary": "Origin" } : {}),
    },
  });
}

function validConfiguredOrigin(value) {
  if (!value || value.includes("YOUR-") || value.includes("REPLACE")) return false;
  try {
    const parsed = new URL(value);
    return parsed.protocol === "https:" && parsed.origin === value;
  } catch {
    return false;
  }
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const configuredOrigin = env.ALLOWED_ORIGIN;
    const requestOrigin = request.headers.get("Origin");

    // Fail closed until a real HTTPS frontend origin has been configured.
    if (!validConfiguredOrigin(configuredOrigin)) {
      return json({ error: "API is not configured" }, 503);
    }

    // Reject browser requests from every origin except the one configured.
    if (requestOrigin) {
      let parsedOrigin;
      try { parsedOrigin = new URL(requestOrigin).origin; } catch { parsedOrigin = ""; }
      if (parsedOrigin !== configuredOrigin) {
        return json({ error: "Origin not allowed" }, 403);
      }
    }
    const corsOrigin = requestOrigin ? configuredOrigin : null;

    if (request.method === "OPTIONS") {
      if (!corsOrigin) return json({ error: "Origin not allowed" }, 403);
      return new Response(null, {
        status: 204,
        headers: {
          ...SECURITY_HEADERS,
          "Access-Control-Allow-Origin": corsOrigin,
          "Access-Control-Allow-Methods": "GET, OPTIONS",
          "Access-Control-Allow-Headers": "Content-Type, Authorization",
          "Access-Control-Max-Age": "300",
          "Vary": "Origin",
        },
      });
    }

    // No write routes are exposed until authentication, authorization,
    // validation, abuse controls and moderation are implemented and tested.
    if (request.method !== "GET") {
      return json({ error: "Method not allowed" }, 405, corsOrigin);
    }

    if (url.pathname === "/api/health") {
      try {
        if (!env.DB) throw new Error("D1 binding missing");
        await env.DB.prepare("SELECT 1 AS ok").first();
        return json({ ok: true, service: "snipz-api", status: "healthy" }, 200, corsOrigin);
      } catch {
        // Do not leak database names, bindings, stack traces or configuration.
        return json({ ok: false, error: "Service unavailable" }, 503, corsOrigin);
      }
    }

    return json({ error: "Not found" }, 404, corsOrigin);
  },
};
