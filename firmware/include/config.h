#pragma once

// ============================================================
// Anthropic API (host and version are fixed; key comes from NVS)
// ============================================================
#define API_HOST        "api.anthropic.com"
#define API_PORT        443
#define API_VERSION     "2023-06-01"

// ============================================================
// Claude.ai Subscription API (internal web API)
// ============================================================
#define CLAUDEAI_HOST       "claude.ai"
#define CLAUDEAI_PROXY_PORT 8081   // Legacy: local proxy fallback

// Default cloud proxy URL — deploy cloud-proxy/ to Vercel and set this.
// Users can override via the web config portal on the device.
// Example: "https://your-project.vercel.app"
#define CLAUDEAI_DEFAULT_PROXY_URL ""

// ============================================================
// WiFi
// ============================================================
#define WIFI_TIMEOUT_MS 15000

// ============================================================
// Refresh Settings
// ============================================================
#define REFRESH_INTERVAL_MS  300000   // 5 minutes
#define NTP_SERVER           "pool.ntp.org"
#define NTP_SYNC_INTERVAL_MS 3600000  // Re-sync NTP every 1 hour

// ============================================================
// Backlight Settings
// ============================================================
#define BACKLIGHT_FULL       255
#define BACKLIGHT_DIM        40
#define DIM_TIMEOUT_MS       120000   // Dim after 2 minutes

// ============================================================
// Reboot delay after web config save (ms)
// ============================================================
#define REBOOT_DELAY_MS      2000
