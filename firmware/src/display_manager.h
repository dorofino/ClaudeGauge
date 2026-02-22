#pragma once

#include <TFT_eSPI.h>
#include "data_models.h"

class DisplayManager {
public:
    void init();
    void showSplash();
    void showConnecting(const char* status);
    void showSetupScreen(const char* apName, const char* ip);
    void setBacklight(uint8_t level);

    TFT_eSPI&    tft()    { return _tft; }
    TFT_eSprite& sprite() { return _sprite; }

    void pushSprite();

private:
    TFT_eSPI    _tft;
    TFT_eSprite _sprite = TFT_eSprite(&_tft);
    uint8_t     _blLevel = 255;
};
