#pragma once

#include <Arduino.h>
#include "pin_config.h"

class ButtonHandler {
public:
    void init();
    void update();

    bool isPrevPressed();       // Previous screen (two-button boards only)
    bool isNextPressed();       // Next screen
    bool isRefreshPressed();    // Force data refresh (long press)
    bool anyPressed();          // Any button event happened

private:
    struct ButtonState {
        uint8_t  pin;
        bool     lastReading;
        bool     stableState;
        bool     pressed;
        bool     longPressed;
        uint32_t lastDebounceTime;
        uint32_t pressStartTime;
        bool     longFired;
    };

#if HAS_TWO_BUTTONS
    ButtonState _btnLeft;
    ButtonState _btnRight;
#else
    ButtonState _btnBoot;
#endif

    static constexpr uint32_t DEBOUNCE_MS    = 50;
    static constexpr uint32_t LONG_PRESS_MS  = 800;

    void updateButton(ButtonState& btn);
};
