#include "setup_screen.h"

void SetupScreen::setApInfo(const char* apName, const char* apIp) {
    strncpy(_apName, apName, sizeof(_apName) - 1);
    strncpy(_apIp, apIp, sizeof(_apIp) - 1);
}

void SetupScreen::onDraw(TFT_eSprite& spr, const LcarsFrame::Rect& full) {
    spr.fillSprite(LCARS_BLACK);

    int16_t cx = full.w / 2;
    int16_t y = 20;

    // Title
    LcarsFont::drawTextUpper(spr, "CLAUDEGAUGE SETUP", cx, y,
                              LCARS_FONT_MD, LCARS_GOLD,
                              LCARS_BLACK, TC_DATUM);
    y += 30;

    // Instructions
    LcarsFont::drawTextUpper(spr, "CONNECT TO WIFI:", cx, y,
                              LCARS_FONT_SM, LCARS_ICE,
                              LCARS_BLACK, TC_DATUM);
    y += 18;

    LcarsFont::drawText(spr, _apName, cx, y,
                         LCARS_FONT_LG, LCARS_AMBER,
                         LCARS_BLACK, TC_DATUM);
    y += 36;

    LcarsFont::drawTextUpper(spr, "THEN OPEN:", cx, y,
                              LCARS_FONT_SM, LCARS_ICE,
                              LCARS_BLACK, TC_DATUM);
    y += 18;

    char urlBuf[32];
    snprintf(urlBuf, sizeof(urlBuf), "HTTP://%s", _apIp);
    LcarsFont::drawText(spr, urlBuf, cx, y,
                         LCARS_FONT_MD, LCARS_AMBER,
                         LCARS_BLACK, TC_DATUM);

    // Blinking indicator at bottom
    int16_t indY = full.h - 20;
    LcarsWidgets::drawIndicator(spr, cx - 30, indY, 4, LCARS_GOLD, true);
    LcarsFont::drawTextUpper(spr, "WAITING", cx + 4, indY,
                              LCARS_FONT_SM, LCARS_GOLD,
                              LCARS_BLACK, ML_DATUM);
}
