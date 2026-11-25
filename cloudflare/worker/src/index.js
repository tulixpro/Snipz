const SECURITY_HEADERS = {
  "X-Content-Type-Options": "nosniff",
  "Referrer-Policy": "strict-origin-when-cross-origin",
  "X-Frame-Options": "DENY",
  "Permissions-Policy": "camera=(), microphone=(), geolocation=()",
  "Cache-Control": "no-store",
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

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const configuredOrigin = env.ALLOWED_ORIGIN;
    const requestOrigin = request.headers.get("Origin");

    if (requestOrigin && (!configuredOrigin || requestOrigin !== configuredOrigin)) {
      return json({ error: "Origin not allowed" }, 403);
    }
    const corsOrigin = requestOrigin && requestOrigin === configuredOrigin ? configuredOrigin : null;

    if (request.method === "OPTIONS") {
      if (!corsOrigin) return json({ error: "Origin not allowed" }, 403);
      return new Response(null, {
        status: 204,
        headers: {
          ...SECURITY_HEADERS,
          "Access-Control-Allow-Origin": corsOrigin,
          "Access-Control-Allow-Methods": "GET, OPTIONS",
          "Access-Control-Allow-Headers": "Content-Type, Authorization",
          "Access-Control-Max-Age": "600",
          "Vary": "Origin",
        },
      });
    }

    if (url.pathname === "/api/health" && request.method === "GET") {
      try {
        await env.DB.prepare("SELECT 1 AS ok").first();
        return json({
          ok: true,
          service: "snipz-api",
          database: "connected",
          videoStorage: Boolean(env.VIDEOS),
        }, 200, corsOrigin);
      } catch {
        return json({ ok: false, error: "Service unavailable" }, 503, corsOrigin);
      }
    }
    return json({ error: "Not found" }, 404, corsOrigin);
  },
};
