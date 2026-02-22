/**
 * Claude.ai API Proxy — Vercel Edge Function
 *
 * Relays Claude.ai API requests from the ESP32.
 * Deploy with: npx vercel --prod
 *
 * Routing is handled via vercel.json rewrites that pass the
 * original path as a ?p= query parameter.
 *
 * The ESP32 calls:
 *   GET https://<project>.vercel.app/api/organizations
 *   GET https://<project>.vercel.app/api/organizations/{uuid}/usage
 *   GET https://<project>.vercel.app/api/organizations/{uuid}/overage_spend_limit
 */

export const config = { runtime: "edge" };

const CLAUDE_HOST = "https://claude.ai";

const BROWSER_HEADERS = {
  accept: "*/*",
  "accept-language": "en-US,en;q=0.9",
  "content-type": "application/json",
  "anthropic-client-platform": "web_claude_ai",
  "anthropic-client-version": "1.0.0",
  "user-agent":
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) " +
    "AppleWebKit/537.36 (KHTML, like Gecko) " +
    "Chrome/131.0.0.0 Safari/537.36",
  origin: "https://claude.ai",
  referer: "https://claude.ai/settings/usage",
  "sec-fetch-dest": "empty",
  "sec-fetch-mode": "cors",
  "sec-fetch-site": "same-origin",
};

export default async function handler(request) {
  const url = new URL(request.url);
  const path = url.searchParams.get("p");

  // Health check
  if (!path || path === "ping") {
    return Response.json({ ok: true });
  }

  // Only allow GET
  if (request.method !== "GET") {
    return new Response("Method not allowed", { status: 405 });
  }

  // Get session key from header
  const sessionKey = request.headers.get("X-Session-Key");
  if (!sessionKey) {
    return Response.json(
      { error: "Missing X-Session-Key header" },
      { status: 401 }
    );
  }

  // Build upstream URL: /api/<captured path>
  // Strip 'p' param, preserve any other query params from the original request
  url.searchParams.delete("p");
  const search = url.search;
  const upstreamUrl = `${CLAUDE_HOST}/api/${path}${search}`;

  try {
    const resp = await fetch(upstreamUrl, {
      method: "GET",
      headers: {
        ...BROWSER_HEADERS,
        Cookie: `sessionKey=${sessionKey}`,
      },
    });

    const body = await resp.arrayBuffer();
    return new Response(body, {
      status: resp.status,
      headers: {
        "Content-Type": "application/json",
        "Access-Control-Allow-Origin": "*",
      },
    });
  } catch (e) {
    return Response.json({ error: e.message }, { status: 502 });
  }
}
