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
#define WIFI_MAX_NETWORKS 3

// ============================================================
// Backlight dimming (overrides lcars-esp32's 2min/40 defaults —
// too aggressive for a desk display with no touch to wake it)
// ============================================================
#define SCREEN_DIM_TIMEOUT_MS  900000  // Dim after 15 minutes idle
#define SCREEN_DIM_BRIGHTNESS  120     // Dim level (0-255), not near-off
#define STR_(x) #x
#define STR(x) STR_(x)

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
