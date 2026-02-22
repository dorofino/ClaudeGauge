#pragma once

#include <lcars.h>

class SetupScreen : public LcarsScreen {
public:
    void setApInfo(const char* apName, const char* apIp);

    const char* title() const override { return ""; }
    bool wantsFrame() const override { return false; }
    uint32_t refreshIntervalMs() const override { return 500; }

    void onDraw(TFT_eSprite& spr, const LcarsFrame::Rect& full) override;

private:
    char _apName[32] = {};
    char _apIp[16] = {};
};
