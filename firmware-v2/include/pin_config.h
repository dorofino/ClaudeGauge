#pragma once

// ============================================================
// Board-specific pin configuration
// ============================================================

#if defined(BOARD_TDISPLAY_S3)

    // Buttons (active LOW, internal pull-up)
    #define BTN_LEFT    0     // GPIO 0  = BOOT button
    #define BTN_RIGHT   14    // GPIO 14 = second button
    #define HAS_TWO_BUTTONS   1

    // Backlight PWM
    #define PIN_BL      38
    #define BL_CHANNEL  0
    #define BL_FREQ     5000
    #define BL_RES      8     // 8-bit resolution (0-255)

    // Power control (T-Display-S3 specific)
    #define PIN_POWER_ON 15
    #define HAS_POWER_PIN 1

    // Capacitive touch (CST816S via I2C)
    #define HAS_TOUCH   1
    #define TOUCH_SDA   18
    #define TOUCH_SCL   17
    #define TOUCH_INT   16
    #define TOUCH_RST   21

#elif defined(BOARD_WAVESHARE_147)

    // Single BOOT button only (active LOW, internal pull-up)
    #define BTN_BOOT    0     // GPIO 0 = BOOT button
    #define HAS_TWO_BUTTONS   0

    // Backlight PWM
    #define PIN_BL      48
    #define BL_CHANNEL  0
    #define BL_FREQ     5000
    #define BL_RES      8     // 8-bit resolution (0-255)

    // No power pin needed
    #define HAS_POWER_PIN 0

    // No touch controller
    #define HAS_TOUCH   0

#else
    #error "No board defined! Add -DBOARD_TDISPLAY_S3 or -DBOARD_WAVESHARE_147 to build_flags"
#endif
