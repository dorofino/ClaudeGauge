#include "ui_renderer.h"
#include "ui_widgets.h"
#include "colors.h"
#include "screen_layouts.h"
#include <Arduino.h>
#include <time.h>

// Content area shortcuts (used for centered overlays)
#define CX CONTENT_X
#define CY CONTENT_Y
#define CW CONTENT_W
#define CH CONTENT_H

// Line spacing for each font size
#define H_SM  16   // small font row height

// Local helper: draw string with smooth font
static void lcarsText(TFT_eSprite& spr, const char* text, int16_t x, int16_t y,
                      const uint8_t* font, uint16_t color,
                      uint8_t datum = TL_DATUM) {
    spr.loadFont(font);
    spr.setTextDatum(datum);
    spr.setTextColor(color, CLR_BG);
    spr.drawString(text, x, y);
    spr.unloadFont();
}

// Section header: small colored dash + label text
static void sectionHeader(TFT_eSprite& spr, int16_t x, int16_t y,
                           const char* label, uint16_t color) {
    spr.fillRect(x, y + 5, 3, 8, color);
    spr.loadFont(LCARS_SM);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(color, CLR_BG);
    spr.drawString(label, x + 6, y);
    spr.unloadFont();
}

// Render custom text elements added via designer toolbox
#ifdef CUSTOM_SCREEN_0_COUNT
static void drawCustomElements(TFT_eSprite& spr, const CustomTextEl* els, int count) {
    for (int i = 0; i < count; i++) {
        lcarsText(spr, els[i].text, els[i].x, els[i].y, els[i].font, els[i].color);
    }
}
#endif

// ================================================================
// Screen 1: OVERVIEW - Cost & Token Usage
// ================================================================
void UIRenderer::drawOverview(TFT_eSprite& spr, const AppState& state,
                               uint32_t countdown) {
    UIWidgets::drawLcarsFrame(spr, "USAGE OVERVIEW",
        SCREEN_OVERVIEW, SCREEN_COUNT, countdown, state.wifi_connected, state.wifi_rssi);

    if (state.is_fetching) {
        lcarsText(spr, "FETCHING DATA", CX + CW / 2, CY + CH / 2 - 12,
                  LCARS_MD, CLR_PEACH, MC_DATUM);
        if (state.fetch_status[0] != '\0') {
            lcarsText(spr, state.fetch_status, CX + CW / 2, CY + CH / 2 + 12,
                      LCARS_SM, CLR_TEXT_DIM, MC_DATUM);
        }
        return;
    }
    if (!state.cost.valid && !state.usage.valid) {
        if (state.api_error) {
            lcarsText(spr, "API ERROR", CX + CW / 2, CY + CH / 2 - 14,
                      LCARS_MD, CLR_RED, MC_DATUM);
            lcarsText(spr, state.last_error, CX + CW / 2, CY + CH / 2 + 14,
                      LCARS_SM, CLR_TEXT_DIM, MC_DATUM);
        } else {
            lcarsText(spr, "AWAITING DATA", CX + CW / 2, CY + CH / 2,
                      LCARS_MD, CLR_PEACH, MC_DATUM);
        }
        return;
    }

    // === Left side: Cost display ===
    UIWidgets::drawLabel(spr, LBL_TODAY_X, LBL_TODAY_Y, "TODAY", CLR_AMBER);
    UIWidgets::drawCostValue(spr, COST_TODAY_X, COST_TODAY_Y, state.cost.today_usd, LCARS_XL, CLR_PEACH);
    UIWidgets::drawLabel(spr, LBL_MONTH_X, LBL_MONTH_Y, "THIS MONTH", CLR_AMBER);
    UIWidgets::drawCostValue(spr, COST_MONTH_X, COST_MONTH_Y, state.cost.month_usd, LCARS_LG, CLR_SKY);

    // === Vertical divider ===
#if defined(DIV_MAIN_X)
    spr.drawFastVLine(DIV_MAIN_X, DIV_MAIN_Y, DIV_MAIN_H, CLR_AMBER);
    spr.drawFastVLine(DIV_MAIN_X + 1, DIV_MAIN_Y, DIV_MAIN_H, CLR_AMBER);
#elif defined(CUSTOM_VLINE_19_X)
    spr.drawFastVLine(CUSTOM_VLINE_19_X, CUSTOM_VLINE_19_Y, CUSTOM_VLINE_19_H, CLR_AMBER);
    spr.drawFastVLine(CUSTOM_VLINE_19_X + 1, CUSTOM_VLINE_19_Y, CUSTOM_VLINE_19_H, CLR_AMBER);
#endif

    // === Right side: Cost Breakdown bars ===
    UIWidgets::drawLabel(spr, LBL_BREAKDOWN_X, LBL_BREAKDOWN_Y, "COST BREAKDOWN", CLR_AMBER);

    if (state.cost.model_count > 0) {
        float maxCost = 0.01f;
        int maxDisplay = min((int)state.cost.model_count, 4);
        for (int i = 0; i < maxDisplay; i++) {
            if (state.cost.models[i].cost_usd > maxCost)
                maxCost = state.cost.models[i].cost_usd;
        }

        static const uint16_t costColors[4] = {
            CLR_AMBER, CLR_LAVENDER, CLR_BLUE, CLR_SALMON
        };

        for (int i = 0; i < maxDisplay; i++) {
            char shortName[16];
            UIWidgets::shortenModelName(state.cost.models[i].model_name,
                                         shortName, sizeof(shortName));
            UIWidgets::drawCostRow(spr, COST_MODEL_0_X,
                COST_MODEL_START_Y + i * COST_MODEL_ROW_STEP,
                COST_MODEL_0_W, shortName,
                state.cost.models[i].cost_usd, maxCost, costColors[i]);
        }
    } else {
        lcarsText(spr, "NO MODEL DATA", COST_MODEL_0_X,
                  COST_MODEL_START_Y + 20, LCARS_SM, CLR_TEXT_DIM);
    }

    #if defined(CUSTOM_SCREEN_0_COUNT) && CUSTOM_SCREEN_0_COUNT > 0
    drawCustomElements(spr, CUSTOM_SCREEN_0, CUSTOM_SCREEN_0_COUNT);
    #endif
}

// ================================================================
// Shared: Model breakdown screen content
// ================================================================
static void drawModelContent(TFT_eSprite& spr, const UsageData& usage) {
    if (!usage.valid || usage.model_count == 0) {
        lcarsText(spr, "NO MODEL DATA", CX + CW / 2, CY + CH / 2,
                  LCARS_MD, CLR_PEACH, MC_DATUM);
        return;
    }

    uint64_t maxTotal = 1;
    for (int i = 0; i < usage.model_count; i++) {
        uint64_t t = usage.models[i].tokens.total();
        if (t > maxTotal) maxTotal = t;
    }

    int maxDisplay = min((int)usage.model_count, 4);
    uint16_t modelColors[5] = { CLR_AMBER, CLR_LAVENDER, CLR_BLUE, CLR_SALMON, CLR_SKY };

    for (int i = 0; i < maxDisplay; i++) {
        int16_t y = TM_MODEL_START_Y + i * TM_MODEL_ROW_STEP;
        const ModelUsage& m = usage.models[i];
        uint16_t mc = modelColors[i % 5];

        String name = String(m.model_name);
        name.replace("claude-", "");

        // Colored indicator + model name
        spr.fillRect(TM_MODEL_0_X, y + 4, 3, 8, mc);
        lcarsText(spr, name.c_str(), TM_MODEL_0_X + 6, y, LCARS_SM, mc);

        // Token counts right-aligned
        String inStr  = UIWidgets::formatCount(m.tokens.totalInput());
        String outStr = UIWidgets::formatCount(m.tokens.output);
        char buf[40];
        snprintf(buf, sizeof(buf), "IN:%s OUT:%s", inStr.c_str(), outStr.c_str());
        lcarsText(spr, buf, TM_MODEL_0_X + TM_MODEL_0_W, y, LCARS_SM, CLR_PEACH, TR_DATUM);

        // Progress bar
        float pct = (float)m.tokens.total() / (float)maxTotal;
        UIWidgets::drawProgressBar(spr, TM_MODEL_0_X, y + H_SM, TM_MODEL_0_W, 4, pct, mc, CLR_BAR_BG);
    }

    if (usage.model_count > 4) {
        int16_t y = TM_MODEL_START_Y + 4 * TM_MODEL_ROW_STEP;
        char buf[24];
        snprintf(buf, sizeof(buf), "+%d more", usage.model_count - 4);
        UIWidgets::drawLabel(spr, TM_MODEL_0_X, y + 2, buf, CLR_TEXT_DIM);
    }
}

// ================================================================
// Shared: Claude Code metrics screen content
// ================================================================
static void drawCodeContent(TFT_eSprite& spr, const ClaudeCodeData& code) {
    if (!code.valid) {
        lcarsText(spr, "NO CODE DATA", CX + CW / 2, CY + CH / 2,
                  LCARS_MD, CLR_PEACH, MC_DATUM);
        return;
    }

    // === Left column ===
    sectionHeader(spr, TC_SEC_SESSIONS_X, TC_SEC_SESSIONS_Y, "SESSIONS", CLR_AMBER);
    lcarsText(spr, String(code.total_sessions).c_str(),
              TC_VAL_SESSIONS_X, TC_VAL_SESSIONS_Y, LCARS_MD, CLR_TEXT_BRIGHT, TR_DATUM);

    sectionHeader(spr, TC_SEC_LINES_X, TC_SEC_LINES_Y, "LINES", CLR_AMBER);
    char linesBuf[24];
    snprintf(linesBuf, sizeof(linesBuf), "+%ld / -%ld",
             (long)code.total_lines_added,
             (long)code.total_lines_removed);
    lcarsText(spr, linesBuf, TC_VAL_LINES_X, TC_VAL_LINES_Y, LCARS_SM, CLR_SKY, TR_DATUM);

    UIWidgets::drawStatusRow(spr, TC_ROW_COMMITS_X, TC_ROW_COMMITS_Y, TC_ROW_COMMITS_W,
        "COMMITS", String(code.total_commits).c_str(), CLR_PEACH);

    UIWidgets::drawStatusRow(spr, TC_ROW_PRS_X, TC_ROW_PRS_Y, TC_ROW_PRS_W,
        "PULL REQS", String(code.total_prs).c_str(), CLR_PEACH);

    UIWidgets::drawLabel(spr, TC_LBL_COST_X, TC_LBL_COST_Y, "EST. COST", CLR_AMBER);
    char costBuf[16];
    snprintf(costBuf, sizeof(costBuf), "$%.2f", code.total_cost);
    lcarsText(spr, costBuf, TC_VAL_COST_X, TC_VAL_COST_Y, LCARS_SM, CLR_SKY, TR_DATUM);

    // === Vertical divider ===
    spr.drawFastVLine(TC_DIV_X, TC_DIV_Y, TC_DIV_H, CLR_AMBER);

    // === Right column: Tool Acceptance ===
    sectionHeader(spr, TC_SEC_ACCEPT_X, TC_SEC_ACCEPT_Y, "TOOL ACCEPTANCE", CLR_AMBER);

    UIWidgets::drawAcceptBar(spr, TC_BAR_EDIT_X, TC_BAR_EDIT_Y, TC_BAR_EDIT_W, "EDIT",
        code.total_edit_accepted,
        code.total_edit_rejected, CLR_SKY);

    UIWidgets::drawAcceptBar(spr, TC_BAR_WRITE_X, TC_BAR_WRITE_Y, TC_BAR_WRITE_W, "WRITE",
        code.total_write_accepted,
        code.total_write_rejected, CLR_LAVENDER);

    spr.drawFastHLine(TC_HLINE_X, TC_HLINE_Y, TC_HLINE_W, CLR_TEXT_DIM);

    sectionHeader(spr, TC_SEC_USERS_X, TC_SEC_USERS_Y, "USERS", CLR_AMBER);

    int maxUsers = min((int)code.user_count, 2);
    for (int i = 0; i < maxUsers; i++) {
        String email = String(code.users[i].email);
        if (email.length() > 14) {
            email = email.substring(0, 11) + "...";
        }
        char userBuf[16];
        snprintf(userBuf, sizeof(userBuf), "$%.2f", code.users[i].estimated_cost);
        UIWidgets::drawStatusRow(spr, TC_ROW_USER_X,
            TC_ROW_USER_Y + i * H_SM,
            TC_ROW_USER_W, email.c_str(), userBuf, CLR_PEACH);
    }
}

// ================================================================
// Screen 2: TODAY MODEL BREAKDOWN
// ================================================================
void UIRenderer::drawModels(TFT_eSprite& spr, const AppState& state,
                             uint32_t countdown) {
    UIWidgets::drawLcarsFrame(spr, "TODAY MODEL ANALYSIS",
        SCREEN_MODELS, SCREEN_COUNT, countdown, state.wifi_connected, state.wifi_rssi);
    if (state.is_fetching) {
        lcarsText(spr, "FETCHING DATA", CX + CW / 2, CY + CH / 2 - 12,
                  LCARS_MD, CLR_PEACH, MC_DATUM);
        if (state.fetch_status[0] != '\0')
            lcarsText(spr, state.fetch_status, CX + CW / 2, CY + CH / 2 + 12,
                      LCARS_SM, CLR_TEXT_DIM, MC_DATUM);
        return;
    }
    drawModelContent(spr, state.usage);
    #if defined(CUSTOM_SCREEN_1_COUNT) && CUSTOM_SCREEN_1_COUNT > 0
    drawCustomElements(spr, CUSTOM_SCREEN_1, CUSTOM_SCREEN_1_COUNT);
    #endif
}

// ================================================================
// Screen 3: MONTHLY MODEL BREAKDOWN
// ================================================================
void UIRenderer::drawMonthlyModels(TFT_eSprite& spr, const AppState& state,
                                    uint32_t countdown) {
    UIWidgets::drawLcarsFrame(spr, "MONTHLY MODELS",
        SCREEN_MODELS_MONTHLY, SCREEN_COUNT, countdown, state.wifi_connected, state.wifi_rssi);
    if (state.is_fetching) {
        lcarsText(spr, "FETCHING DATA", CX + CW / 2, CY + CH / 2 - 12,
                  LCARS_MD, CLR_PEACH, MC_DATUM);
        if (state.fetch_status[0] != '\0')
            lcarsText(spr, state.fetch_status, CX + CW / 2, CY + CH / 2 + 12,
                      LCARS_SM, CLR_TEXT_DIM, MC_DATUM);
        return;
    }
    drawModelContent(spr, state.monthly_usage);
    #if defined(CUSTOM_SCREEN_2_COUNT) && CUSTOM_SCREEN_2_COUNT > 0
    drawCustomElements(spr, CUSTOM_SCREEN_2, CUSTOM_SCREEN_2_COUNT);
    #endif
}

// ================================================================
// Screen 4: TODAY CLAUDE CODE METRICS
// ================================================================
void UIRenderer::drawClaudeCode(TFT_eSprite& spr, const AppState& state,
                                 uint32_t countdown) {
    UIWidgets::drawLcarsFrame(spr, "TODAY CODE ANALYTICS",
        SCREEN_CODE, SCREEN_COUNT, countdown, state.wifi_connected, state.wifi_rssi);
    if (state.is_fetching) {
        lcarsText(spr, "FETCHING DATA", CX + CW / 2, CY + CH / 2 - 12,
                  LCARS_MD, CLR_PEACH, MC_DATUM);
        if (state.fetch_status[0] != '\0')
            lcarsText(spr, state.fetch_status, CX + CW / 2, CY + CH / 2 + 12,
                      LCARS_SM, CLR_TEXT_DIM, MC_DATUM);
        return;
    }
    drawCodeContent(spr, state.code);
    #if defined(CUSTOM_SCREEN_3_COUNT) && CUSTOM_SCREEN_3_COUNT > 0
    drawCustomElements(spr, CUSTOM_SCREEN_3, CUSTOM_SCREEN_3_COUNT);
    #endif
}

// ================================================================
// Screen 5: MONTHLY CLAUDE CODE METRICS
// ================================================================
void UIRenderer::drawMonthlyCode(TFT_eSprite& spr, const AppState& state,
                                  uint32_t countdown) {
    UIWidgets::drawLcarsFrame(spr, "MONTHLY CODE",
        SCREEN_CODE_MONTHLY, SCREEN_COUNT, countdown, state.wifi_connected, state.wifi_rssi);
    if (state.is_fetching) {
        lcarsText(spr, "FETCHING DATA", CX + CW / 2, CY + CH / 2 - 12,
                  LCARS_MD, CLR_PEACH, MC_DATUM);
        if (state.fetch_status[0] != '\0')
            lcarsText(spr, state.fetch_status, CX + CW / 2, CY + CH / 2 + 12,
                      LCARS_SM, CLR_TEXT_DIM, MC_DATUM);
        return;
    }
    drawCodeContent(spr, state.monthly_code);
    #if defined(CUSTOM_SCREEN_4_COUNT) && CUSTOM_SCREEN_4_COUNT > 0
    drawCustomElements(spr, CUSTOM_SCREEN_4, CUSTOM_SCREEN_4_COUNT);
    #endif
}

// ================================================================
// Screen 6: STATUS / DIAGNOSTICS
// ================================================================
void UIRenderer::drawStatus(TFT_eSprite& spr, const AppState& state,
                              uint32_t countdown) {
    UIWidgets::drawLcarsFrame(spr, "SYSTEM STATUS",
        SCREEN_STATUS, SCREEN_COUNT, countdown, state.wifi_connected, state.wifi_rssi);

    // === Left column: Network + API ===
    sectionHeader(spr, ST_SEC_NET_X, ST_SEC_NET_Y, "NETWORK", CLR_AMBER);

    UIWidgets::drawStatusRow(spr, ST_WIFI_X, ST_WIFI_Y, ST_WIFI_W, "WiFi",
        state.wifi_connected ? "CONNECTED" : "OFFLINE",
        state.wifi_connected ? CLR_STATUS_OK : CLR_STATUS_ERR);

    UIWidgets::drawStatusRow(spr, ST_IP_X, ST_IP_Y, ST_IP_W, "IP",
        state.wifi_ip, CLR_PEACH);

    char rssiBuf[16];
    snprintf(rssiBuf, sizeof(rssiBuf), "%d dBm", state.wifi_rssi);
    UIWidgets::drawStatusRow(spr, ST_SIGNAL_X, ST_SIGNAL_Y, ST_SIGNAL_W, "Signal",
        rssiBuf, CLR_PEACH);
    UIWidgets::drawSignalBars(spr, ST_BARS_X, ST_BARS_Y, state.wifi_rssi);

    sectionHeader(spr, ST_SEC_API_X, ST_SEC_API_Y, "API", CLR_LAVENDER);

    const char* statusVal = "OK";
    uint16_t statusClr = CLR_STATUS_OK;
    if (state.api_error) {
        statusVal = (state.last_error[0] != '\0') ? state.last_error : "ERROR";
        statusClr = CLR_STATUS_ERR;
    }
    UIWidgets::drawStatusRow(spr, ST_API_X, ST_API_Y, ST_API_W, "Status", statusVal, statusClr);

    uint32_t nowMs = millis();
    uint32_t ageSec = state.last_refresh > 0
        ? (nowMs - state.last_refresh) / 1000 : 0;
    char ageBuf[16];
    if (ageSec < 60) snprintf(ageBuf, sizeof(ageBuf), "%lus ago", (unsigned long)ageSec);
    else snprintf(ageBuf, sizeof(ageBuf), "%lum ago", (unsigned long)(ageSec / 60));
    UIWidgets::drawStatusRow(spr, ST_FETCHED_X, ST_FETCHED_Y, ST_FETCHED_W, "Fetched", ageBuf, CLR_PEACH);

    // === Vertical divider ===
    spr.drawFastVLine(ST_DIV_X, ST_DIV_Y, ST_DIV_H, CLR_AMBER);

    // === Right column: System + Firmware ===
    sectionHeader(spr, ST_SEC_SYS_X, ST_SEC_SYS_Y, "SYSTEM", CLR_BLUE);

    char heapBuf[16];
    snprintf(heapBuf, sizeof(heapBuf), "%lu KB", (unsigned long)(ESP.getFreeHeap() / 1024));
    UIWidgets::drawStatusRow(spr, ST_HEAP_X, ST_HEAP_Y, ST_HEAP_W, "Heap", heapBuf, CLR_PEACH);

    char psramBuf[16];
    snprintf(psramBuf, sizeof(psramBuf), "%lu KB", (unsigned long)(ESP.getFreePsram() / 1024));
    UIWidgets::drawStatusRow(spr, ST_PSRAM_X, ST_PSRAM_Y, ST_PSRAM_W, "PSRAM", psramBuf, CLR_PEACH);

    uint32_t uptimeSec = (nowMs - state.uptime_start) / 1000;
    char uptimeBuf[16];
    snprintf(uptimeBuf, sizeof(uptimeBuf), "%luh %lum",
             (unsigned long)(uptimeSec / 3600),
             (unsigned long)((uptimeSec % 3600) / 60));
    UIWidgets::drawStatusRow(spr, ST_UPTIME_X, ST_UPTIME_Y, ST_UPTIME_W, "Uptime", uptimeBuf, CLR_PEACH);

    sectionHeader(spr, ST_SEC_FW_X, ST_SEC_FW_Y, "FIRMWARE", CLR_SALMON);

    UIWidgets::drawLabel(spr, ST_FW_VER_X, ST_FW_VER_Y, "LCARS v2.0", CLR_TEXT_DIM);
#if defined(BOARD_TDISPLAY_S3)
    UIWidgets::drawLabel(spr, ST_BOARD_X, ST_BOARD_Y, "T-Display-S3", CLR_TEXT_DIM);
#elif defined(BOARD_WAVESHARE_147)
    UIWidgets::drawLabel(spr, ST_BOARD_X, ST_BOARD_Y, "Waveshare 1.47", CLR_TEXT_DIM);
#else
    UIWidgets::drawLabel(spr, ST_BOARD_X, ST_BOARD_Y, "ESP32-S3", CLR_TEXT_DIM);
#endif

    #if defined(CUSTOM_SCREEN_5_COUNT) && CUSTOM_SCREEN_5_COUNT > 0
    drawCustomElements(spr, CUSTOM_SCREEN_5, CUSTOM_SCREEN_5_COUNT);
    #endif
}

// ================================================================
// Helper: Draw a limit row (label, reset time, segmented bar, %)
// ================================================================
// Draw a donut/arc gauge: filled arc from top (270°) clockwise
static void drawDonutGauge(TFT_eSprite& spr, int16_t cx, int16_t cy,
                            int16_t r, int16_t thickness,
                            float pct, uint16_t fgColor, uint16_t bgColor,
                            const char* pctText,
                            const uint8_t* pctFont = LCARS_MD,
                            uint16_t pctColor = CLR_TEXT_BRIGHT) {
    if (pct > 1.0f) pct = 1.0f;
    if (pct < 0.0f) pct = 0.0f;

    // Background arc (full circle)
    spr.drawSmoothArc(cx, cy, r, r - thickness, 0, 360, bgColor, CLR_BG, true);

    // Filled arc — starts at top (270° in screen coords) going clockwise
    if (pct > 0.01f) {
        int16_t endAngle = (int16_t)(pct * 360.0f);
        // drawSmoothArc uses 0=top, angles go clockwise
        // We need to map: 0% = 0°, 100% = 360°
        if (endAngle > 0) {
            // TFT_eSPI drawSmoothArc: start/end in degrees, 0=right, 90=bottom
            // To start from top: startAngle=270 (which is top in standard coords)
            int16_t sa = 270;
            int16_t ea = sa + endAngle;
            if (ea >= 360) {
                // Draw two segments: 270-360 and 0-(ea-360)
                spr.drawSmoothArc(cx, cy, r, r - thickness, sa, 360, fgColor, CLR_BG, true);
                int16_t remainder = ea - 360;
                if (remainder > 0)
                    spr.drawSmoothArc(cx, cy, r, r - thickness, 0, remainder, fgColor, CLR_BG, true);
            } else {
                spr.drawSmoothArc(cx, cy, r, r - thickness, sa, ea, fgColor, CLR_BG, true);
            }
        }
    }

    // Percentage text centered in donut
    spr.loadFont(pctFont);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(pctColor, CLR_BG);
    spr.drawString(pctText, cx, cy - 2);
    spr.unloadFont();
}

// Draw a compact limit bar (no percentage text)
static void drawLimitBar(TFT_eSprite& spr, int16_t x, int16_t y, int16_t w,
                          const char* label, float utilization, uint16_t color) {
    // Label
    spr.fillRect(x, y + 4, 3, 8, color);
    spr.loadFont(LCARS_SM);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(color, CLR_BG);
    spr.drawString(label, x + 6, y);
    spr.unloadFont();

    // Segmented bar
    int16_t barY = y + H_SM;
    int16_t barH = 5;
    int16_t segW = 4, gap = 1;
    float pct = utilization / 100.0f;
    if (pct > 1.0f) pct = 1.0f;

    int nSegs = 0;
    for (int16_t tx = x; tx + segW <= x + w; tx += segW + gap) nSegs++;
    int filledSegs = (int)(nSegs * pct + 0.5f);

    int16_t sx = x;
    int seg = 0;
    while (sx + segW <= x + w) {
        spr.fillRect(sx, barY, segW, barH, (seg < filledSegs) ? color : CLR_BAR_BG);
        sx += segW + gap;
        seg++;
    }
}

// Draw a vertical countdown bar (fills from bottom)
static void drawCountdownBar(TFT_eSprite& spr, int16_t x, int16_t y,
                              int16_t w, int16_t h, float pct, uint16_t color) {
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    spr.fillRect(x, y, w, h, CLR_BAR_BG);
    int16_t fillH = (int16_t)(h * pct);
    if (fillH > 0)
        spr.fillRect(x, y + h - fillH, w, fillH, color);
}

// Format seconds remaining as countdown text with live seconds
static void formatResetCountdown(char* buf, size_t len, int32_t secs) {
    if (secs <= 0) { snprintf(buf, len, "0:00"); return; }
    int32_t d = secs / 86400;
    int32_t h = (secs % 86400) / 3600;
    int32_t m = (secs % 3600) / 60;
    int32_t s = secs % 60;
    if (d > 0)      snprintf(buf, len, "%ldd %ldh%ldm", d, h, m);
    else if (h > 0) snprintf(buf, len, "%ld:%02ld:%02ld", h, m, s);
    else             snprintf(buf, len, "%ld:%02ld", m, s);
}

// ================================================================
// Screen 7: CLAUDE.AI SUBSCRIPTION USAGE
// ================================================================
void UIRenderer::drawClaudeAi(TFT_eSprite& spr, const AppState& state,
                                uint32_t countdown) {
    UIWidgets::drawLcarsFrame(spr, "CLAUDE.AI USAGE",
        SCREEN_CLAUDEAI, SCREEN_COUNT, countdown, state.wifi_connected, state.wifi_rssi);

    const ClaudeAiUsage& cai = state.claude_ai;

    if (state.is_fetching) {
        lcarsText(spr, "FETCHING DATA", CX + CW / 2, CY + CH / 2 - 12,
                  LCARS_MD, CLR_PEACH, MC_DATUM);
        if (state.fetch_status[0] != '\0') {
            lcarsText(spr, state.fetch_status, CX + CW / 2, CY + CH / 2 + 12,
                      LCARS_SM, CLR_TEXT_DIM, MC_DATUM);
        }
        return;
    }
    if (!cai.valid) {
        if (cai.error[0] != '\0') {
            lcarsText(spr, "SESSION ERROR", CX + CW / 2, CY + CH / 2 - 14,
                      LCARS_MD, CLR_RED, MC_DATUM);
            lcarsText(spr, cai.error, CX + CW / 2, CY + CH / 2 + 14,
                      LCARS_SM, CLR_TEXT_DIM, MC_DATUM);
        } else {
            lcarsText(spr, "NOT CONFIGURED", CX + CW / 2, CY + CH / 2 - 14,
                      LCARS_MD, CLR_PEACH, MC_DATUM);
            char setupMsg[40];
            if (state.wifi_ip[0] != '\0') {
                snprintf(setupMsg, sizeof(setupMsg), "Visit http://%s", state.wifi_ip);
            } else {
                snprintf(setupMsg, sizeof(setupMsg), "Set session in web portal");
            }
            lcarsText(spr, setupMsg, CX + CW / 2, CY + CH / 2 + 14,
                      LCARS_SM, CLR_TEXT_DIM, MC_DATUM);
        }
        return;
    }

    // Layout uses positions from screen_layouts.h (set in designer)
    // Fallback defaults for donut pct font/color (generated on save)
    #ifndef CAI_5H_PCT_FONT
    #define CAI_5H_PCT_FONT  LCARS_MD
    #endif
    #ifndef CAI_5H_PCT_COLOR
    #define CAI_5H_PCT_COLOR  CLR_TEXT_BRIGHT
    #endif
    #ifndef CAI_7D_PCT_FONT
    #define CAI_7D_PCT_FONT  LCARS_MD
    #endif
    #ifndef CAI_7D_PCT_COLOR
    #define CAI_7D_PCT_COLOR  CLR_TEXT_BRIGHT
    #endif
    // Get current time for countdown calculations
    time_t now;
    time(&now);

    // Left donut: 5-hour (CAI_5H_X/Y is the center)
    if (cai.five_hour.present) {
        int16_t r5h = CAI_5H_W / 2;
        char pctBuf[8];
        snprintf(pctBuf, sizeof(pctBuf), "%.0f%%", cai.five_hour.utilization);
        drawDonutGauge(spr, CAI_5H_X, CAI_5H_Y, r5h, CAI_5H_T,
                       cai.five_hour.utilization / 100.0f,
                       CLR_SKY, CLR_BAR_BG, pctBuf,
                       CAI_5H_PCT_FONT, CAI_5H_PCT_COLOR);
        // Countdown label (separate text element positioned from designer)
        #if defined(CAI_5H_LABEL_X)
        int32_t remaining5h = (int32_t)(cai.five_hour.resets_at - now);
        char cdLabel5h[16];
        formatResetCountdown(cdLabel5h, sizeof(cdLabel5h), remaining5h);
        #ifndef CAI_5H_LABEL_DATUM
        #define CAI_5H_LABEL_DATUM TC_DATUM
        #endif
        lcarsText(spr, cdLabel5h, CAI_5H_LABEL_X, CAI_5H_LABEL_Y,
                  CAI_5H_LABEL_FONT, CAI_5H_LABEL_COLOR, CAI_5H_LABEL_DATUM);
        #endif
        // Vertical countdown bar
        #if defined(CAI_5H_CD_X)
        int32_t rem5h = (int32_t)(cai.five_hour.resets_at - now);
        float cdPct5h = rem5h > 0 ? (float)rem5h / 18000.0f : 0.0f;
        drawCountdownBar(spr, CAI_5H_CD_X, CAI_5H_CD_Y,
                         CAI_5H_CD_W, CAI_5H_CD_H, cdPct5h, CLR_SKY);
        #endif
    }

    // Right donut: 7-day (CAI_7D_X/Y is the center)
    if (cai.seven_day.present) {
        int16_t r7d = CAI_7D_W / 2;
        char pctBuf[8];
        snprintf(pctBuf, sizeof(pctBuf), "%.0f%%", cai.seven_day.utilization);
        drawDonutGauge(spr, CAI_7D_X, CAI_7D_Y, r7d, CAI_7D_T,
                       cai.seven_day.utilization / 100.0f,
                       CLR_LAVENDER, CLR_BAR_BG, pctBuf,
                       CAI_7D_PCT_FONT, CAI_7D_PCT_COLOR);
        // Countdown label (separate text element positioned from designer)
        #if defined(CAI_7D_LABEL_X)
        int32_t remaining7d = (int32_t)(cai.seven_day.resets_at - now);
        char cdLabel7d[16];
        formatResetCountdown(cdLabel7d, sizeof(cdLabel7d), remaining7d);
        #ifndef CAI_7D_LABEL_DATUM
        #define CAI_7D_LABEL_DATUM TC_DATUM
        #endif
        lcarsText(spr, cdLabel7d, CAI_7D_LABEL_X, CAI_7D_LABEL_Y,
                  CAI_7D_LABEL_FONT, CAI_7D_LABEL_COLOR, CAI_7D_LABEL_DATUM);
        #endif
        // Vertical countdown bar
        #if defined(CAI_7D_CD_X)
        int32_t rem7d = (int32_t)(cai.seven_day.resets_at - now);
        float cdPct7d = rem7d > 0 ? (float)rem7d / 604800.0f : 0.0f;
        drawCountdownBar(spr, CAI_7D_CD_X, CAI_7D_CD_Y,
                         CAI_7D_CD_W, CAI_7D_CD_H, cdPct7d, CLR_LAVENDER);
        #endif
    }

    // Bars from designer positions
    #if defined(CAI_OPUS_X)
    if (cai.seven_day_opus.present) {
        drawLimitBar(spr, CAI_OPUS_X, CAI_OPUS_Y, CAI_OPUS_W, "OPUS",
                     cai.seven_day_opus.utilization, CLR_AMBER);
    }
    #endif

    #if defined(CAI_SONNET_X)
    if (cai.seven_day_sonnet.present) {
        drawLimitBar(spr, CAI_SONNET_X, CAI_SONNET_Y, CAI_SONNET_W, "SONNET",
                     cai.seven_day_sonnet.utilization, CLR_BLUE);
    }
    #endif

    // Vertical divider between donuts and bars
    #if defined(CUSTOM_VLINE_7_X)
    spr.drawFastVLine(CUSTOM_VLINE_7_X, CUSTOM_VLINE_7_Y, CUSTOM_VLINE_7_H, CLR_AMBER);
    #elif defined(CUSTOM_VLINE_18_X)
    spr.drawFastVLine(CUSTOM_VLINE_18_X, CUSTOM_VLINE_18_Y, CUSTOM_VLINE_18_H, CLR_AMBER);
    #endif

    // Extra usage bar
    #if defined(CAI_EXTRA_X)
    if (cai.extra_enabled) {
        char spendBuf[24];
        snprintf(spendBuf, sizeof(spendBuf), "EXTRA $%u/$%u",
                 cai.extra_used_cents / 100, cai.extra_limit_cents / 100);
        float extraPct = cai.extra_limit_cents > 0
            ? (float)cai.extra_used_cents / (float)cai.extra_limit_cents : 0;
        drawLimitBar(spr, CAI_EXTRA_X, CAI_EXTRA_Y, CAI_EXTRA_W, spendBuf, extraPct * 100.0f, CLR_SALMON);
    }
    #endif

    #if defined(CUSTOM_SCREEN_6_COUNT) && CUSTOM_SCREEN_6_COUNT > 0
    drawCustomElements(spr, CUSTOM_SCREEN_6, CUSTOM_SCREEN_6_COUNT);
    #endif
}
