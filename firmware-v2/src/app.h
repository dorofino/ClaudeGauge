#pragma once

#include <Arduino.h>

// ============================================================
// ClaudeGauge v2 — Application State
// Phase 1: Claude.ai subscription tracking only
// ============================================================

#define ERROR_MSG_LEN    64
#define IP_LEN           16

// ── Claude.ai Rate Limit ─────────────────────────────────────

struct ClaudeAiLimit {
    float    utilization;    // 0-100 percentage
    uint32_t resets_at;      // Unix epoch seconds
    bool     present;        // false if null in JSON response
};

struct ClaudeAiUsage {
    ClaudeAiLimit five_hour;
    ClaudeAiLimit seven_day;
    ClaudeAiLimit seven_day_opus;
    ClaudeAiLimit seven_day_sonnet;
    // Extra usage (overage spend)
    bool     extra_enabled;
    uint32_t extra_limit_cents;   // monthly limit in cents
    uint32_t extra_used_cents;    // spent so far in cents
    // Organization
    char     org_uuid[48];
    bool     valid;
    uint32_t fetched_at;
    char     error[ERROR_MSG_LEN];
};

// ── Application State ────────────────────────────────────────

struct AppState {
    ClaudeAiUsage  claude_ai;

    // WiFi status
    bool           wifi_connected;
    int16_t        wifi_rssi;
    char           wifi_ip[IP_LEN];

    // Fetch state
    bool           is_fetching;
    char           fetch_status[32];

    // Timing
    uint32_t       last_refresh;
    uint32_t       next_refresh;
    uint32_t       uptime_start;
    uint32_t       last_activity;
};
