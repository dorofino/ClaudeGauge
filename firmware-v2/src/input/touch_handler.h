#pragma once

#include <Arduino.h>
#include "pin_config.h"

#if HAS_TOUCH
class TouchHandler {
public:
    void init();
    void update();

    bool isDoubleTapped();  // double-tap detected this frame
    bool isTouched();       // any touch event (for wake/activity)

private:
    static constexpr uint8_t CST816S_ADDR     = 0x15;
    static constexpr uint32_t DOUBLE_TAP_MS   = 400;  // max gap between taps
    static constexpr uint32_t TAP_DEBOUNCE_MS = 50;

    bool     _doubleTapped  = false;
    bool     _touched       = false;
    bool     _wasTouching   = false;
    uint8_t  _tapCount      = 0;
    uint32_t _lastTapTime   = 0;

    void     resetChip();
    uint8_t  readRegister(uint8_t reg);
    bool     readTouch();
};
#endif
