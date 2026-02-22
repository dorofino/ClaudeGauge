#include "touch_handler.h"
#include "pin_config.h"

#if HAS_TOUCH
#include <Wire.h>

void TouchHandler::init() {
    // Reset the CST816S
    resetChip();

    // Initialize I2C on touch-specific pins
    Wire.begin(TOUCH_SDA, TOUCH_SCL);

    // Configure interrupt pin
    pinMode(TOUCH_INT, INPUT);

    // Verify chip is present
    Wire.beginTransmission(CST816S_ADDR);
    if (Wire.endTransmission() == 0) {
        Serial.println("CST816S touch controller found");
    } else {
        Serial.println("CST816S touch controller NOT found");
    }
}

void TouchHandler::resetChip() {
    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    delay(10);
    digitalWrite(TOUCH_RST, HIGH);
    delay(50);
}

uint8_t TouchHandler::readRegister(uint8_t reg) {
    Wire.beginTransmission(CST816S_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(CST816S_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

bool TouchHandler::readTouch() {
    uint8_t points = readRegister(0x02);  // Number of touch points
    return (points > 0);
}

void TouchHandler::update() {
    _doubleTapped = false;
    _touched = false;

    bool touching = readTouch();
    uint32_t now = millis();

    // Detect finger-down edge (transition from not touching to touching)
    if (touching && !_wasTouching) {
        _touched = true;

        if (_tapCount > 0 && (now - _lastTapTime) <= DOUBLE_TAP_MS) {
            // Second tap within window -> double tap
            _doubleTapped = true;
            _tapCount = 0;
        } else {
            // First tap or tap after timeout
            _tapCount = 1;
            _lastTapTime = now;
        }
    }

    // Reset tap count if too much time passed since last tap
    if (_tapCount > 0 && (now - _lastTapTime) > DOUBLE_TAP_MS) {
        _tapCount = 0;
    }

    _wasTouching = touching;
}

bool TouchHandler::isDoubleTapped() {
    return _doubleTapped;
}

bool TouchHandler::isTouched() {
    return _touched;
}
#endif
