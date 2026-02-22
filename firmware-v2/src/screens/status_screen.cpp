#include "status_screen.h"
#include "config.h"
#include <WiFi.h>

void StatusScreen::onDraw(TFT_eSprite& spr, const LcarsFrame::Rect& c) {
    int16_t x = c.x;
    int16_t y = c.y;
    int16_t w = c.w;

    // WiFi status
    const char* wifiStatus = _state.wifi_connected ? "CONNECTED" : "DISCONNECTED";
    uint16_t wifiColor = _state.wifi_connected ? _theme->statusOk : _theme->statusErr;
    LcarsWidgets::drawStatusRow(spr, x, y, w, "WIFI", wifiStatus,
                                 wifiColor, _theme->textDim);
    y += 15;

    // RSSI
    if (_state.wifi_connected) {
        char rssiBuf[16];
        snprintf(rssiBuf, sizeof(rssiBuf), "%d DBM", _state.wifi_rssi);
        LcarsWidgets::drawStatusRow(spr, x, y, w, "SIGNAL", rssiBuf,
                                     _theme->statusOk, _theme->textDim);
        y += 15;

        // IP address
        LcarsWidgets::drawStatusRow(spr, x, y, w, "IP", _state.wifi_ip,
                                     _theme->accent, _theme->textDim);
        y += 15;
    }

    // Uptime
    uint32_t upSec = (millis() - _state.uptime_start) / 1000;
    uint32_t upH = upSec / 3600;
    uint32_t upM = (upSec % 3600) / 60;
    char uptimeBuf[16];
    snprintf(uptimeBuf, sizeof(uptimeBuf), "%luH %luM", (unsigned long)upH, (unsigned long)upM);
    LcarsWidgets::drawStatusRow(spr, x, y, w, "UPTIME", uptimeBuf,
                                 _theme->accent, _theme->textDim);
    y += 15;

    // Next refresh countdown
    if (_state.next_refresh > 0) {
        uint32_t now = millis();
        int32_t remaining = ((int32_t)_state.next_refresh - (int32_t)now) / 1000;
        if (remaining < 0) remaining = 0;
        char refreshBuf[16];
        snprintf(refreshBuf, sizeof(refreshBuf), "%lds", (long)remaining);
        LcarsWidgets::drawStatusRow(spr, x, y, w, "REFRESH IN", refreshBuf,
                                     _theme->accent, _theme->textDim);
        y += 15;
    }

    // Firmware version
    LcarsWidgets::drawStatusRow(spr, x, y, w, "FIRMWARE", FW_VERSION,
                                 _theme->textDim, _theme->textDim);
    y += 17;

    // Data cascade fills remaining space
    LcarsWidgets::drawSeparator(spr, x, y, w, _theme->textDim);
    y += 3;
    int16_t cascadeH = c.y + c.h - y;
    if (cascadeH > 8) {
        LcarsWidgets::drawDataCascade(spr, x, y, w, cascadeH, _theme->accent);
    }
}
