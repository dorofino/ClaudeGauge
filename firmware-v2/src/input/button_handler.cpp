#include "button_handler.h"

void ButtonHandler::init() {
#if HAS_TWO_BUTTONS
    _btnLeft  = { BTN_LEFT,  true, true, false, false, 0, 0, false };
    _btnRight = { BTN_RIGHT, true, true, false, false, 0, 0, false };
    pinMode(BTN_LEFT,  INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
#else
    _btnBoot = { BTN_BOOT, true, true, false, false, 0, 0, false };
    pinMode(BTN_BOOT, INPUT_PULLUP);
#endif
}

void ButtonHandler::update() {
#if HAS_TWO_BUTTONS
    _btnLeft.pressed     = false;
    _btnLeft.longPressed = false;
    _btnRight.pressed     = false;
    _btnRight.longPressed = false;
    updateButton(_btnLeft);
    updateButton(_btnRight);
#else
    _btnBoot.pressed     = false;
    _btnBoot.longPressed = false;
    updateButton(_btnBoot);
#endif
}

void ButtonHandler::updateButton(ButtonState& btn) {
    bool reading = digitalRead(btn.pin);
    uint32_t now = millis();

    // Debounce
    if (reading != btn.lastReading) {
        btn.lastDebounceTime = now;
    }
    btn.lastReading = reading;

    if ((now - btn.lastDebounceTime) < DEBOUNCE_MS) {
        return;
    }

    // State change detection
    if (reading != btn.stableState) {
        btn.stableState = reading;

        if (!reading) {
            // Button pressed (active LOW)
            btn.pressStartTime = now;
            btn.longFired = false;
        } else {
            // Button released
            if (!btn.longFired) {
                btn.pressed = true;  // short press
            }
        }
    }

    // Long press detection (while held)
    if (!btn.stableState && !btn.longFired) {
        if ((now - btn.pressStartTime) >= LONG_PRESS_MS) {
            btn.longPressed = true;
            btn.longFired = true;
        }
    }
}

bool ButtonHandler::isPrevPressed() {
#if HAS_TWO_BUTTONS
    return _btnLeft.pressed;
#else
    return false;  // No previous on single-button boards
#endif
}

bool ButtonHandler::isNextPressed() {
#if HAS_TWO_BUTTONS
    return _btnRight.pressed;
#else
    return _btnBoot.pressed;
#endif
}

bool ButtonHandler::isRefreshPressed() {
#if HAS_TWO_BUTTONS
    return _btnRight.longPressed;
#else
    return _btnBoot.longPressed;
#endif
}

bool ButtonHandler::anyPressed() {
#if HAS_TWO_BUTTONS
    return _btnLeft.pressed || _btnRight.pressed ||
           _btnLeft.longPressed || _btnRight.longPressed;
#else
    return _btnBoot.pressed || _btnBoot.longPressed;
#endif
}
