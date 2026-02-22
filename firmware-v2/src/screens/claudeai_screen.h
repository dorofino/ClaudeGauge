#pragma once

#include <lcars.h>
#include "app.h"

class ClaudeAiScreen : public LcarsScreen {
public:
    ClaudeAiScreen(const AppState& state) : _state(state) {}

    const char* title() const override { return "CLAUDE.AI"; }
    uint32_t refreshIntervalMs() const override { return 100; }  // Live countdown

    void setScreenInfo(uint8_t idx, uint8_t total) { _screenIdx = idx; _totalScreens = total; }

    void onDraw(TFT_eSprite& spr, const LcarsFrame::Rect& c) override;

private:
    const AppState& _state;
    uint8_t _screenIdx = 0;
    uint8_t _totalScreens = 2;

    // Content helpers
    void _formatCountdown(uint32_t resets_at, char* buf, size_t len);
    void _drawLoading(TFT_eSprite& spr, const LcarsFrame::Rect& c);
    void _drawError(TFT_eSprite& spr, const LcarsFrame::Rect& c);

    // Frame chrome helpers (draw on engine's bars)
    void _drawChrome(TFT_eSprite& spr);
    void _drawSparkIcon(TFT_eSprite& spr, int16_t cx, int16_t cy, uint16_t color);
    void _drawSignalBars(TFT_eSprite& spr, int16_t x, int16_t y);
    void _drawRefreshPill(TFT_eSprite& spr, int16_t x, int16_t y, int16_t w, int16_t h);
};
