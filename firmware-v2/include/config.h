#pragma once

// ============================================================
// Claude.ai Subscription API (via Vercel proxy)
// ============================================================
#define CLAUDEAI_HOST       "claude.ai"
// Deploy cloud-proxy/ to Vercel and set your URL here, or configure via the device web portal.
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
// Reboot delay after web config save (ms)
// ============================================================
#define REBOOT_DELAY_MS      2000

// ============================================================
// Firmware version
// ============================================================
#define FW_VERSION "2.0.0"
