#pragma once

#include <lcars.h>
#include "app.h"

class StatusScreen : public LcarsScreen {
public:
    StatusScreen(const AppState& state) : _state(state) {}

    const char* title() const override { return "SYSTEM STATUS"; }
    uint32_t refreshIntervalMs() const override { return 80; }

    void onDraw(TFT_eSprite& spr, const LcarsFrame::Rect& c) override;

private:
    const AppState& _state;
};
