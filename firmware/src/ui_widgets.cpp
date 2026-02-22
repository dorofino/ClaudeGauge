#include "ui_widgets.h"
#include "colors.h"
#include "screen_layouts.h"
#include "config.h"

// Helper: load smooth font, draw string, unload
static void smoothText(TFT_eSprite& spr, const char* text, int16_t x, int16_t y,
                        const uint8_t* font, uint16_t color, uint8_t datum = TL_DATUM) {
    spr.loadFont(font);
    spr.setTextDatum(datum);
    spr.setTextColor(color, CLR_BG);
    spr.drawString(text, x, y);
    spr.unloadFont();
}

// Variant with custom background color (for text on colored chips)
static void smoothTextBg(TFT_eSprite& spr, const char* text, int16_t x, int16_t y,
                          const uint8_t* font, uint16_t color, uint16_t bgColor,
                          uint8_t datum = TL_DATUM) {
    spr.loadFont(font);
    spr.setTextDatum(datum);
    spr.setTextColor(color, bgColor);
    spr.drawString(text, x, y);
    spr.unloadFont();
}

// ============================================================
// LCARS Elbow Drawing
// ============================================================

static void fillQuarterCircle(TFT_eSprite& spr, int16_t cx, int16_t cy,
                               int16_t r, uint16_t color, uint8_t quadrant) {
    for (int16_t y = 0; y <= r; y++) {
        int16_t x = (int16_t)sqrtf((float)(r * r - y * y));
        switch (quadrant) {
            case 0: spr.drawFastHLine(cx - x, cy - y, x, color); break;
            case 1: spr.drawFastHLine(cx, cy - y, x, color); break;
            case 2: spr.drawFastHLine(cx, cy + y, x, color); break;
            case 3: spr.drawFastHLine(cx - x, cy + y, x, color); break;
        }
    }
}

// ============================================================
// LCARS Frame
// ============================================================

void UIWidgets::drawLcarsFrame(TFT_eSprite& spr, const char* title,
                                uint8_t currentPage, uint8_t totalPages,
                                uint32_t countdownSec, bool wifiOk,
                                int16_t wifiRssi) {
    spr.fillSprite(CLR_BG);

    const int16_t SW = SIDEBAR_W;
    const int16_t R  = ELBOW_R;
    const int16_t TH = TOPBAR_H;
    const int16_t BH = BOTBAR_H;

    bool blink = (millis() / 500) % 2;

    // ========== TOP-LEFT ELBOW ==========
    spr.fillRect(0, 0, SW, TH + R, CLR_AMBER);
    spr.fillRect(SW, 0, R, TH, CLR_AMBER);
    fillQuarterCircle(spr, SW, TH, R, CLR_BG, 2);

    // ========== SIDEBAR SEGMENTS ==========
    int16_t segTop = TH + R + 2;
    int16_t segBot = SCR_H - BH - R - 2;
    int16_t segH   = segBot - segTop;
    uint16_t segColors[3] = { CLR_SALMON, CLR_LAVENDER, CLR_BLUE };
    const char* segNums[3] = { "01", "02", "03" };
    int16_t eachH = (segH - 4) / 3;

    for (int i = 0; i < 3; i++) {
        int16_t sy = segTop + i * (eachH + 2);
        spr.fillRect(0, sy, SW - 4, eachH, segColors[i]);
        spr.fillRoundRect(SW - 8, sy, 8, eachH, 4, segColors[i]);
        spr.setTextDatum(TL_DATUM);
        spr.setTextColor(CLR_BG, segColors[i]);
        spr.drawString(segNums[i], 2, sy + eachH / 2 - 4, 1);
    }

    // ========== BOTTOM-LEFT ELBOW ==========
    spr.fillRect(0, SCR_H - BH - R, SW, BH + R, CLR_TAN);
    spr.fillRect(SW, SCR_H - BH, R, BH, CLR_TAN);
    fillQuarterCircle(spr, SW, SCR_H - BH, R, CLR_BG, 1);

    // ========== TOP BAR ==========
    int16_t barX = SW + R + 4;
    int16_t barY = 1;
    int16_t barH = TH - 2;

    // Title left-aligned, vertically centered in top bar gap
    smoothText(spr, title, barX, barY + barH / 2 - 1, LCARS_MD, CLR_AMBER, ML_DATUM);

    // Second segment: peach pill-bar on the right (square left, rounded right cap)
    const int16_t seg2X = 210;
    const int16_t seg2R = barH / 2;
    spr.fillRoundRect(seg2X, barY, SCR_W - seg2X, barH, seg2R, CLR_PEACH);
    spr.fillRect(seg2X, barY, seg2R, barH, CLR_PEACH);  // Square off left edge
    // "CLAUDE.AI" branding on the peach bar (black on peach)
    smoothTextBg(spr, "CLAUDE.AI", SCR_W - 6, barY + barH / 2 - 1,
                 LCARS_14, CLR_BG, CLR_PEACH, MR_DATUM);

    // Scan line animation in the gap (sweeps left to right)
    {
        const int16_t gapEnd = seg2X - 4;
        const int16_t scanW = gapEnd - barX;
        if (scanW > 0) {
            float phase = (float)(millis() % 3000) / 3000.0f;
            int16_t scanX = barX + (int16_t)(phase * scanW);
            spr.fillRect(scanX, barY + barH - 2, 3, 2, CLR_AMBER);
        }
    }

    // ========== BOTTOM BAR ==========
    barY = SCR_H - BH + 1;
    barH = BH - 2;

    // Page chip
    char pageBuf[8];
    snprintf(pageBuf, sizeof(pageBuf), "%d/%d", currentPage + 1, totalPages);
    spr.fillRoundRect(FRAME_PAGE_X, FRAME_PAGE_Y, FRAME_PAGE_W, FRAME_PAGE_H, FRAME_PAGE_H / 2, FRAME_PAGE_COLOR);
    spr.fillRect(FRAME_PAGE_X, FRAME_PAGE_Y, FRAME_PAGE_H / 2, FRAME_PAGE_H, FRAME_PAGE_COLOR);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(CLR_BG, FRAME_PAGE_COLOR);
    spr.drawString(pageBuf, FRAME_PAGE_X + FRAME_PAGE_W / 2, FRAME_PAGE_Y + FRAME_PAGE_H / 2, 1);

    // Signal bars + label
    drawSignalBars(spr, FRAME_SIGNAL_BARS_X, FRAME_SIGNAL_BARS_Y, wifiRssi);
#if FRAME_SIGNAL_TEXT_FONT == 1
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(FRAME_SIGNAL_TEXT_COLOR, CLR_BG);
    spr.drawString(FRAME_SIGNAL_TEXT_TEXT, FRAME_SIGNAL_TEXT_X, FRAME_SIGNAL_TEXT_Y, 1);
#else
    smoothText(spr, FRAME_SIGNAL_TEXT_TEXT, FRAME_SIGNAL_TEXT_X, FRAME_SIGNAL_TEXT_Y, FRAME_SIGNAL_TEXT_FONT, FRAME_SIGNAL_TEXT_COLOR, MR_DATUM);
#endif

    // Countdown pill with LCARS segmented progress bar
    {
        const int16_t cx = FRAME_COUNTDOWN_X;
        const int16_t cy = FRAME_COUNTDOWN_Y;
        const int16_t cw = FRAME_COUNTDOWN_W;
        const int16_t ch = FRAME_COUNTDOWN_H;
        const int16_t cr = ch / 2;

        // Pill background
        spr.fillRoundRect(cx, cy, cw, ch, cr, FRAME_COUNTDOWN_COLOR);
        spr.fillRect(cx, cy, cr, ch, FRAME_COUNTDOWN_COLOR);  // square left edge

        // "REFRESH IN" label
        spr.setTextDatum(ML_DATUM);
        spr.setTextColor(CLR_BG, FRAME_COUNTDOWN_COLOR);
        spr.drawString("REFRESH IN", cx + 4, cy + ch / 2, 1);

        // LCARS segmented progress bar
        const int16_t barX = cx + 64;
        const int16_t barH = ch - 4;
        const int16_t barY = cy + 2;
        const int16_t barEnd = cx + cw - cr - 1;  // stay inside rounded right edge
        const int16_t segW = 4;   // segment width
        const int16_t gap = 1;    // gap between segments
        const uint32_t totalSec = REFRESH_INTERVAL_MS / 1000;
        float pct = (totalSec > 0 && countdownSec <= totalSec)
                    ? 1.0f - (float)countdownSec / (float)totalSec : 1.0f;

        int16_t sx = barX;
        int nSegs = 0;
        // Count total segments
        for (int16_t tx = barX; tx + segW <= barEnd; tx += segW + gap) nSegs++;
        int filledSegs = (int)(nSegs * pct + 0.5f);

        int seg = 0;
        while (sx + segW <= barEnd) {
            uint16_t segColor = (seg < filledSegs) ? CLR_PEACH : CLR_BG;
            spr.fillRect(sx, barY, segW, barH, segColor);
            sx += segW + gap;
            seg++;
        }
    }

    // ========== SCANNING INDICATOR ==========
    int16_t scanRange = segBot - segTop;
    int16_t scanPos = segTop + (int16_t)((millis() / 20) % scanRange);
    spr.fillRect(SW - 3, scanPos, 3, 2, CLR_PEACH);
}

// ============================================================
// Progress Bar
// ============================================================

void UIWidgets::drawProgressBar(TFT_eSprite& spr, int16_t x, int16_t y,
                                 int16_t w, int16_t h, float percent,
                                 uint16_t fgColor, uint16_t bgColor) {
    float clamped = constrain(percent, 0.0f, 1.0f);
    int16_t fillW = (int16_t)(w * clamped);
    spr.fillRoundRect(x, y, w, h, h / 2, bgColor);
    if (fillW > h)
        spr.fillRoundRect(x, y, fillW, h, h / 2, fgColor);
    else if (fillW > 0)
        spr.fillCircle(x + h / 2, y + h / 2, h / 2, fgColor);
}

// ============================================================
// Token Row  (two lines: label+value, then bar below)
// Total height: ~22px (14px text area + 4px bar + gap)
// ============================================================

void UIWidgets::drawTokenRow(TFT_eSprite& spr, int16_t x, int16_t y,
                              int16_t barW, const char* label, uint64_t value,
                              uint64_t maxValue, uint16_t color) {
    // Colored dot
    spr.fillCircle(x + 2, y + 5, 2, color);

    // Label (shift up 3px to align middle with dot)
    smoothText(spr, label, x + 10, y - 3, LCARS_SM, color);

    // Value right-aligned on same line
    String valStr = formatCount(value);
    smoothText(spr, valStr.c_str(), x + barW, y - 3, LCARS_SM, CLR_PEACH, TR_DATUM);

    // Progress bar below text (ascent=14 + 3px gap)
    float pct = maxValue > 0 ? (float)value / (float)maxValue : 0;
    drawProgressBar(spr, x, y + 17, barW, 4, pct, color, CLR_BAR_BG);
}

// ============================================================
// Signal Bars
// ============================================================

void UIWidgets::drawSignalBars(TFT_eSprite& spr, int16_t x, int16_t y,
                                int16_t rssi) {
    int bars = 0;
    if (rssi > -55) bars = 4;
    else if (rssi > -65) bars = 3;
    else if (rssi > -75) bars = 2;
    else if (rssi > -85) bars = 1;

    for (int i = 0; i < 4; i++) {
        int16_t barH = 3 + i * 3;
        int16_t barX = x + i * 5;
        int16_t barY = y + 12 - barH;
        spr.fillRect(barX, barY, 3, barH, (i < bars) ? CLR_STATUS_OK : CLR_TEXT_DIM);
    }
}

// ============================================================
// Separator
// ============================================================

void UIWidgets::drawSeparator(TFT_eSprite& spr, int16_t y) {
    spr.drawFastHLine(CONTENT_X, y, CONTENT_W, CLR_TEXT_DIM);
}

// ============================================================
// Format Count
// ============================================================

String UIWidgets::formatCount(uint64_t value) {
    if (value >= 1000000000ULL) return String((float)value / 1000000000.0f, 1) + "B";
    if (value >= 1000000ULL)    return String((float)value / 1000000.0f, 1) + "M";
    if (value >= 1000ULL)       return String((float)value / 1000.0f, 1) + "K";
    return String((uint32_t)value);
}

// ============================================================
// Cost Value
// ============================================================

void UIWidgets::drawCostValue(TFT_eSprite& spr, int16_t x, int16_t y,
                               float cost, const uint8_t* font, uint16_t color) {
    char buf[20];
    if (cost >= 1000.0f)
        snprintf(buf, sizeof(buf), "$%.0f", cost);
    else
        snprintf(buf, sizeof(buf), "$%.2f", cost);
    smoothText(spr, buf, x, y, font, color);
}

// ============================================================
// Label
// ============================================================

void UIWidgets::drawLabel(TFT_eSprite& spr, int16_t x, int16_t y,
                           const char* text, uint16_t color, const uint8_t* font) {
    smoothText(spr, text, x, y, font, color);
}

// ============================================================
// Accept Bar  (height ~14px: label + percent + bar inline)
// ============================================================

void UIWidgets::drawAcceptBar(TFT_eSprite& spr, int16_t x, int16_t y,
                               int16_t barW, const char* label,
                               uint16_t accepted, uint16_t rejected,
                               uint16_t color) {
    uint16_t total = accepted + rejected;
    float pct = total > 0 ? (float)accepted / (float)total : 0;

    smoothText(spr, label, x, y, LCARS_SM, CLR_PEACH);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", total > 0 ? (int)(pct * 100) : 0);
    smoothText(spr, buf, x + 42, y, LCARS_SM, CLR_TEXT_BRIGHT);

    drawProgressBar(spr, x + 72, y + 3, barW - 72, 6, pct, color, CLR_BAR_BG);
}

// ============================================================
// Status Row  (height ~14px)
// ============================================================

void UIWidgets::drawStatusRow(TFT_eSprite& spr, int16_t x, int16_t y,
                               int16_t w, const char* label, const char* value,
                               uint16_t valueColor) {
    smoothText(spr, label, x, y, LCARS_SM, CLR_LAVENDER);
    smoothText(spr, value, x + w, y, LCARS_SM, valueColor, TR_DATUM);
}

// ============================================================
// Cost Row  (same layout as Token Row: dot + label + $value + bar)
// ============================================================

void UIWidgets::drawCostRow(TFT_eSprite& spr, int16_t x, int16_t y,
                             int16_t barW, const char* label, float costUsd,
                             float maxCost, uint16_t color) {
    // Colored dot
    spr.fillCircle(x + 2, y + 5, 2, color);

    // Label
    smoothText(spr, label, x + 10, y - 3, LCARS_SM, color);

    // Cost value right-aligned
    char buf[16];
    if (costUsd >= 100.0f)
        snprintf(buf, sizeof(buf), "$%.0f", costUsd);
    else if (costUsd >= 10.0f)
        snprintf(buf, sizeof(buf), "$%.1f", costUsd);
    else
        snprintf(buf, sizeof(buf), "$%.2f", costUsd);
    smoothText(spr, buf, x + barW, y - 3, LCARS_SM, CLR_PEACH, TR_DATUM);

    // Progress bar below text
    float pct = maxCost > 0.0f ? costUsd / maxCost : 0.0f;
    drawProgressBar(spr, x, y + 17, barW, 4, pct, color, CLR_BAR_BG);
}

// ============================================================
// Shorten Model Name  (e.g. "claude-sonnet-4-20250514" -> "Sonnet 4")
// ============================================================

void UIWidgets::shortenModelName(const char* fullName, char* out, size_t outLen) {
    struct Family { const char* key; const char* display; };
    static const Family families[] = {
        {"sonnet", "Sonnet"},
        {"haiku",  "Haiku"},
        {"opus",   "Opus"},
    };

    String name(fullName);

    // Detect family
    const char* familyDisplay = nullptr;
    for (const auto& f : families) {
        if (name.indexOf(f.key) >= 0) {
            familyDisplay = f.display;
            break;
        }
    }

    if (!familyDisplay) {
        // Unknown model — strip "claude-" prefix and truncate
        if (name.startsWith("claude-")) name = name.substring(7);
        if ((int)name.length() >= (int)outLen) name = name.substring(0, outLen - 1);
        strncpy(out, name.c_str(), outLen);
        out[outLen - 1] = '\0';
        return;
    }

    // Extract version digits (skip "claude", family keyword, and 8-digit dates)
    String version = "";
    int start = 0;
    while (start < (int)name.length()) {
        int dash = name.indexOf('-', start);
        if (dash < 0) dash = name.length();
        String seg = name.substring(start, dash);
        start = dash + 1;

        if (seg == "claude" || seg.equalsIgnoreCase(familyDisplay)) continue;
        if (seg.length() >= 8) continue;  // date like 20250514

        bool isNum = true;
        for (unsigned int i = 0; i < seg.length(); i++) {
            if (!isDigit(seg[i])) { isNum = false; break; }
        }
        if (isNum && seg.length() > 0) {
            if (version.length() > 0) version += ".";
            version += seg;
        }
    }

    snprintf(out, outLen, "%s %s", familyDisplay, version.c_str());
    out[outLen - 1] = '\0';
}
