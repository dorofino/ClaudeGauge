#include "claudeai_screen.h"
#include "screen_layouts.h"
#include <time.h>

// ============================================================
// Layout constants from generated header (see designer/layouts.json v2)
// Gauge colors are dynamic (alarm levels), not from header
// ============================================================

// ============================================================
// Main draw
// ============================================================

void ClaudeAiScreen::onDraw(TFT_eSprite& spr, const LcarsFrame::Rect& c) {
    // Always draw frame chrome (on engine's bars)
    _drawChrome(spr);

    if (_state.is_fetching) {
        _drawLoading(spr, c);
        return;
    }

    const auto& cai = _state.claude_ai;

    if (!cai.valid) {
        _drawError(spr, c);
        return;
    }

    time_t now;
    time(&now);

    // ── LEFT: 5-hour limit ──────────────────────────────────
    if (cai.five_hour.present) {
        float pct5h = cai.five_hour.utilization / 100.0f;
        if (pct5h > 1.0f) pct5h = 1.0f;
        uint16_t color5h = (pct5h > 0.8f) ? LCARS_TOMATO : (pct5h > 0.5f) ? LCARS_AMBER : LCARS_ICE;

        // Countdown clock
        char clockBuf[16];
        _formatCountdown(cai.five_hour.resets_at, clockBuf, sizeof(clockBuf));
        LcarsFont::drawText(spr, clockBuf, V2_5H_CLOCK_X, V2_5H_CLOCK_Y,
                            V2_5H_CLOCK_FONT, V2_5H_CLOCK_COLOR, LCARS_BLACK, V2_5H_CLOCK_DATUM);

        // Donut gauge
        LcarsWidgets::drawDonutGauge(spr, V2_CAI_5H_X, V2_CAI_5H_Y, V2_CAI_5H_R, V2_CAI_5H_T,
                                      pct5h, color5h, LCARS_BAR_TRACK);

        // Percentage text centered in donut
        char pctBuf[8];
        snprintf(pctBuf, sizeof(pctBuf), "%.0f%%", cai.five_hour.utilization);
        LcarsFont::drawText(spr, pctBuf, V2_CAI_5H_X, V2_CAI_5H_Y - 2,
                            V2_CAI_5H_PCT_FONT, V2_CAI_5H_PCT_COLOR, LCARS_BLACK, MC_DATUM);

        // Vertical countdown bar (fills bottom-up based on time remaining)
        int32_t rem5h = (int32_t)cai.five_hour.resets_at - (int32_t)now;
        float cdPct5h = (rem5h > 0) ? (float)rem5h / 18000.0f : 0.0f;
        if (cdPct5h > 1.0f) cdPct5h = 1.0f;
        spr.fillRect(V2_5H_BAR_X, V2_5H_BAR_Y, V2_5H_BAR_W, V2_5H_BAR_H, LCARS_BAR_TRACK);
        int16_t fillH5 = (int16_t)(V2_5H_BAR_H * cdPct5h);
        if (fillH5 > 0)
            spr.fillRect(V2_5H_BAR_X, V2_5H_BAR_Y + V2_5H_BAR_H - fillH5, V2_5H_BAR_W, fillH5, color5h);
    }

    // ── RIGHT: 7-day limit ──────────────────────────────────
    if (cai.seven_day.present) {
        float pct7d = cai.seven_day.utilization / 100.0f;
        if (pct7d > 1.0f) pct7d = 1.0f;
        uint16_t color7d = (pct7d > 0.8f) ? LCARS_TOMATO : (pct7d > 0.5f) ? LCARS_AMBER : LCARS_LAVENDER;

        // Countdown clock
        char clockBuf[16];
        _formatCountdown(cai.seven_day.resets_at, clockBuf, sizeof(clockBuf));
        LcarsFont::drawText(spr, clockBuf, V2_7D_CLOCK_X, V2_7D_CLOCK_Y,
                            V2_7D_CLOCK_FONT, V2_7D_CLOCK_COLOR, LCARS_BLACK, V2_7D_CLOCK_DATUM);

        // Donut gauge
        LcarsWidgets::drawDonutGauge(spr, V2_CAI_7D_X, V2_CAI_7D_Y, V2_CAI_7D_R, V2_CAI_7D_T,
                                      pct7d, color7d, LCARS_BAR_TRACK);

        // Percentage text
        char pctBuf[8];
        snprintf(pctBuf, sizeof(pctBuf), "%.0f%%", cai.seven_day.utilization);
        LcarsFont::drawText(spr, pctBuf, V2_CAI_7D_X, V2_CAI_7D_Y - 2,
                            V2_CAI_7D_PCT_FONT, V2_CAI_7D_PCT_COLOR, LCARS_BLACK, MC_DATUM);

        // Vertical countdown bar
        int32_t rem7d = (int32_t)cai.seven_day.resets_at - (int32_t)now;
        float cdPct7d = (rem7d > 0) ? (float)rem7d / 604800.0f : 0.0f;
        if (cdPct7d > 1.0f) cdPct7d = 1.0f;
        spr.fillRect(V2_7D_BAR_X, V2_7D_BAR_Y, V2_7D_BAR_W, V2_7D_BAR_H, LCARS_BAR_TRACK);
        int16_t fillH7 = (int16_t)(V2_7D_BAR_H * cdPct7d);
        if (fillH7 > 0)
            spr.fillRect(V2_7D_BAR_X, V2_7D_BAR_Y + V2_7D_BAR_H - fillH7, V2_7D_BAR_W, fillH7, color7d);
    }

    // ── Vertical divider ────────────────────────────────────
    spr.fillRect(V2_DIV_X, V2_DIV_Y, 2, V2_DIV_H, V2_DIV_COLOR);
}

// ============================================================
// Countdown format: H:MM:SS for <24h, Xd YhZm for >=24h
// ============================================================

void ClaudeAiScreen::_formatCountdown(uint32_t resets_at, char* buf, size_t len) {
    if (resets_at == 0) {
        snprintf(buf, len, "--:--");
        return;
    }

    time_t now;
    time(&now);
    int32_t remaining = (int32_t)resets_at - (int32_t)now;

    if (remaining <= 0) {
        snprintf(buf, len, "0:00");
        return;
    }

    uint32_t days = remaining / 86400;
    uint32_t hours = (remaining % 86400) / 3600;
    uint32_t mins = (remaining % 3600) / 60;
    uint32_t secs = remaining % 60;

    if (days > 0) {
        snprintf(buf, len, "%lud %luh%lum", (unsigned long)days, (unsigned long)hours, (unsigned long)mins);
    } else if (hours > 0) {
        snprintf(buf, len, "%lu:%02lu:%02lu", (unsigned long)hours, (unsigned long)mins, (unsigned long)secs);
    } else {
        snprintf(buf, len, "%lu:%02lu", (unsigned long)mins, (unsigned long)secs);
    }
}

// ============================================================
// Frame Chrome — drawn on top of engine's bars
// ============================================================

void ClaudeAiScreen::_drawChrome(TFT_eSprite& spr) {
    // ── Top bar: "CLAUDE.AI" branding on amber bar ─────────────
    LcarsFont::drawTextUpper(spr, "CLAUDE.AI", SCR_WIDTH - 12, LCARS_TOPBAR_H / 2,
                              LCARS_FONT_14, LCARS_PEACH, _theme->barTop, MR_DATUM);

    // ── Bottom bar: page chip + signal + refresh pill ────────
    int16_t bbarY = SCR_HEIGHT - LCARS_BOTBAR_H + 1;
    int16_t bbarH = LCARS_BOTBAR_H - 2;

    // Black out the entire bottom bar area (library draws TAN bar behind)
    int16_t barStartX = LCARS_SIDEBAR_W + LCARS_ELBOW_R + LCARS_BAR_GAP;
    spr.fillRect(barStartX, SCR_HEIGHT - LCARS_BOTBAR_H,
                 SCR_WIDTH - barStartX, LCARS_BOTBAR_H, LCARS_BLACK);

    // Page chip (rounded pill with "1/2" text)
    int16_t chipX = FRAME_PAGE_X;
    int16_t chipW = FRAME_PAGE_W;
    spr.fillSmoothRoundRect(chipX, bbarY, chipW, bbarH, bbarH / 2, FRAME_PAGE_COLOR, _theme->barBottom);
    spr.fillRect(chipX, bbarY, bbarH / 2, bbarH, FRAME_PAGE_COLOR);  // Square off left edge

    char pageBuf[8];
    snprintf(pageBuf, sizeof(pageBuf), "%d/%d", _screenIdx + 1, _totalScreens);
    spr.setTextFont(1);  // Built-in 8px font
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(LCARS_BLACK, FRAME_PAGE_COLOR);
    spr.drawString(pageBuf, chipX + chipW / 2, bbarY + bbarH / 2);

    // "SIGNAL" label + signal bars
    spr.setTextFont(FRAME_SIGNAL_TEXT_FONT);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(FRAME_SIGNAL_TEXT_COLOR, LCARS_BLACK);
    spr.drawString(FRAME_SIGNAL_TEXT_TEXT, FRAME_SIGNAL_TEXT_X, bbarY + bbarH / 2);
    _drawSignalBars(spr, FRAME_SIGNAL_BARS_X, bbarY + 1);

    // Refresh countdown pill
    _drawRefreshPill(spr, FRAME_COUNTDOWN_X, bbarY, FRAME_COUNTDOWN_W, bbarH);
}

void ClaudeAiScreen::_drawSparkIcon(TFT_eSprite& spr, int16_t cx, int16_t cy, uint16_t color) {
    spr.fillCircle(cx, cy, 3, color);
    // Cross rays
    spr.fillRect(cx - 1, cy - 7, 2, 3, color);  // Top
    spr.fillRect(cx - 1, cy + 5, 2, 3, color);  // Bottom
    spr.fillRect(cx - 7, cy - 1, 3, 2, color);  // Left
    spr.fillRect(cx + 5, cy - 1, 3, 2, color);  // Right
    // Diagonal rays
    spr.drawLine(cx - 5, cy - 5, cx - 3, cy - 3, color);
    spr.drawLine(cx + 3, cy - 3, cx + 5, cy - 5, color);
    spr.drawLine(cx - 5, cy + 5, cx - 3, cy + 3, color);
    spr.drawLine(cx + 3, cy + 3, cx + 5, cy + 5, color);
}

void ClaudeAiScreen::_drawSignalBars(TFT_eSprite& spr, int16_t x, int16_t y) {
    int16_t rssi = _state.wifi_rssi;
    int bars = 0;
    if (_state.wifi_connected) {
        if (rssi > -55) bars = 4;
        else if (rssi > -65) bars = 3;
        else if (rssi > -75) bars = 2;
        else if (rssi > -85) bars = 1;
    }

    int16_t barHeights[] = { 3, 6, 9, 12 };
    int16_t baseY = y + 12;  // Bottom alignment
    for (int i = 0; i < 4; i++) {
        uint16_t c = (i < bars) ? LCARS_ICE : LCARS_TEXT_DIM;
        spr.fillRect(x + i * 5, baseY - barHeights[i], 3, barHeights[i], c);
    }
}

void ClaudeAiScreen::_drawRefreshPill(TFT_eSprite& spr, int16_t x, int16_t y,
                                       int16_t w, int16_t h) {
    int16_t r = h / 2;

    // Pill background
    spr.fillSmoothRoundRect(x, y, w, h, r, FRAME_COUNTDOWN_COLOR, LCARS_BLACK);
    spr.fillRect(x, y, r, h, FRAME_COUNTDOWN_COLOR);  // Square off left edge

    // "REFRESH IN" label
    spr.setTextFont(1);
    spr.setTextDatum(ML_DATUM);
    spr.setTextColor(LCARS_BLACK, FRAME_COUNTDOWN_COLOR);
    spr.drawString("REFRESH IN", x + 4, y + h / 2);

    // Segmented progress bar inside pill
    int16_t barStartX = x + 64;
    int16_t barEndX = x + w - r - 1;
    int16_t segW = 4, gap = 1;
    int16_t barH = h - 4;
    int16_t barY = y + 2;

    // Calculate refresh progress
    uint32_t now = millis();
    float pct = 0.0f;
    if (_state.next_refresh > _state.last_refresh) {
        uint32_t total = _state.next_refresh - _state.last_refresh;
        uint32_t elapsed = (now > _state.last_refresh) ? (now - _state.last_refresh) : 0;
        pct = (float)elapsed / (float)total;
        if (pct > 1.0f) pct = 1.0f;
    }

    // Count segments
    int nSegs = 0;
    for (int16_t sx = barStartX; sx + segW <= barEndX; sx += segW + gap) nSegs++;
    int filled = (int)(nSegs * pct + 0.5f);

    // Draw segments
    int16_t sx = barStartX;
    int seg = 0;
    while (sx + segW <= barEndX) {
        uint16_t c = (seg < filled) ? LCARS_PEACH : LCARS_BLACK;
        spr.fillRect(sx, barY, segW, barH, c);
        sx += segW + gap;
        seg++;
    }
}

// ============================================================
// Loading / Error states
// ============================================================

void ClaudeAiScreen::_drawLoading(TFT_eSprite& spr, const LcarsFrame::Rect& c) {
    LcarsFont::drawTextUpper(spr, _state.fetch_status,
                              c.x + c.w / 2, c.y + c.h / 2 - 6,
                              LCARS_FONT_SM, _theme->accent,
                              LCARS_BLACK, TC_DATUM);
}

void ClaudeAiScreen::_drawError(TFT_eSprite& spr, const LcarsFrame::Rect& c) {
    const char* msg = _state.claude_ai.error[0] ? _state.claude_ai.error : "NOT CONFIGURED";
    LcarsFont::drawTextUpper(spr, msg,
                              c.x + c.w / 2, c.y + c.h / 2 - 6,
                              LCARS_FONT_SM, LCARS_TOMATO,
                              LCARS_BLACK, TC_DATUM);
}
