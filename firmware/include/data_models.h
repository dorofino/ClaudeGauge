#pragma once

#include <Arduino.h>

#define MAX_MODELS       8
#define MAX_CC_USERS     5
#define MODEL_NAME_LEN   32
#define EMAIL_LEN        48
#define ERROR_MSG_LEN    64
#define IP_LEN           16

// ==================== Usage Data ====================

struct TokenUsage {
    uint64_t uncached_input;
    uint64_t cached_input;
    uint64_t cache_creation;
    uint64_t output;

    uint64_t totalInput() const {
        return uncached_input + cached_input + cache_creation;
    }

    uint64_t total() const {
        return totalInput() + output;
    }
};

struct ModelUsage {
    char       model_name[MODEL_NAME_LEN];
    TokenUsage tokens;
    float      estimated_cost;   // from cost or claude code API
};

struct UsageData {
    TokenUsage  today_total;
    ModelUsage  models[MAX_MODELS];
    uint8_t     model_count;
    bool        valid;
    uint32_t    fetched_at;      // millis() timestamp
};

// ==================== Cost Data ====================

struct ModelCost {
    char  model_name[MODEL_NAME_LEN];
    float cost_usd;
};

struct CostData {
    float      today_usd;
    float      month_usd;
    ModelCost  models[MAX_MODELS];
    uint8_t    model_count;
    bool       valid;
    uint32_t   fetched_at;
};

// ==================== Claude Code Data ====================

struct ClaudeCodeUser {
    char     email[EMAIL_LEN];
    uint16_t sessions;
    int32_t  lines_added;
    int32_t  lines_removed;
    uint16_t commits;
    uint16_t pull_requests;
    uint16_t edit_accepted;
    uint16_t edit_rejected;
    uint16_t write_accepted;
    uint16_t write_rejected;
    float    estimated_cost;
};

struct ClaudeCodeData {
    ClaudeCodeUser users[MAX_CC_USERS];
    uint8_t        user_count;
    // Aggregated totals
    uint32_t total_sessions;
    int32_t  total_lines_added;
    int32_t  total_lines_removed;
    uint32_t total_commits;
    uint32_t total_prs;
    uint16_t total_edit_accepted;
    uint16_t total_edit_rejected;
    uint16_t total_write_accepted;
    uint16_t total_write_rejected;
    float    total_cost;
    // Per-model cost breakdown (from model_breakdown)
    ModelCost model_costs[MAX_MODELS];
    uint8_t   model_cost_count;
    bool     valid;
    uint32_t fetched_at;
};

// ==================== Claude.ai Subscription Usage ====================

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

// ==================== Screen IDs ====================

enum ScreenID : uint8_t {
    SCREEN_OVERVIEW        = 0,
    SCREEN_MODELS          = 1,
    SCREEN_MODELS_MONTHLY  = 2,
    SCREEN_CODE            = 3,
    SCREEN_CODE_MONTHLY    = 4,
    SCREEN_STATUS          = 5,
    SCREEN_CLAUDEAI        = 6,
    SCREEN_COUNT           = 7
};

// ==================== App State ====================

struct AppState {
    ScreenID       current_screen;
    UsageData      usage;            // today
    UsageData      monthly_usage;    // month-to-date
    CostData       cost;
    ClaudeCodeData code;             // today
    ClaudeCodeData monthly_code;     // month-to-date

    // WiFi status
    bool           wifi_connected;
    int16_t        wifi_rssi;
    char           wifi_ip[IP_LEN];

    // API status
    bool           api_error;
    char           last_error[ERROR_MSG_LEN];

    // Timing
    uint32_t       last_refresh;
    uint32_t       next_refresh;
    uint32_t       uptime_start;

    // Claude.ai subscription
    ClaudeAiUsage  claude_ai;

    // Display
    uint8_t        backlight_level;
    uint32_t       last_activity;  // for auto-dim
    bool           is_fetching;    // show loading indicator
    char           fetch_status[32]; // current fetch step message
};
