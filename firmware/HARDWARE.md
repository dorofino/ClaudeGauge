# Hardware Reference

ClaudeGauge supports multiple ESP32-S3 boards. Board-specific configuration is handled via build flags and conditional compilation — see `include/pin_config.h` for feature flags.

---

## LILYGO T-Display-S3

PlatformIO environment: `tdisplays3`

| Spec | Value |
|------|-------|
| MCU | ESP32-S3 (Xtensa LX7 dual-core, 240 MHz) |
| Flash | 16 MB (QSPI) |
| PSRAM | 8 MB (OPI) |
| Display | 1.9" ST7789V IPS, 170x320 pixels, 262K colors |
| Display Interface | 8-bit parallel |
| Interface | USB-C (CDC serial, power, programming) |
| Buttons | 2 tactile buttons |
| Touch | CST816S capacitive (I2C) |
| WiFi | 802.11 b/g/n (2.4 GHz) |
| Bluetooth | BLE 5.0 (not used in this project) |

### Pin Configuration

Defined in `include/pin_config.h` when `BOARD_TDISPLAY_S3` is set:

#### Display (8-bit Parallel)

| Function | GPIO | Notes |
|----------|------|-------|
| CS (Chip Select) | 6 | |
| DC (Data/Command) | 7 | |
| RST (Reset) | 5 | |
| WR (Write Clock) | 8 | |
| RD (Read) | 9 | |
| D0 | 39 | Data bus bit 0 |
| D1 | 40 | Data bus bit 1 |
| D2 | 41 | Data bus bit 2 |
| D3 | 42 | Data bus bit 3 |
| D4 | 45 | Data bus bit 4 |
| D5 | 46 | Data bus bit 5 |
| D6 | 47 | Data bus bit 6 |
| D7 | 48 | Data bus bit 7 |
| BL (Backlight) | 38 | PWM controlled (channel 0, 5 kHz, 8-bit) |

#### Buttons

| Button | GPIO | Configuration |
|--------|------|---------------|
| Left (BOOT) | 0 | Active LOW, internal pull-up |
| Right | 14 | Active LOW, internal pull-up |

#### Touch (CST816S via I2C)

| Function | GPIO |
|----------|------|
| SDA | 18 |
| SCL | 17 |
| INT | 16 |
| RST | 21 |

#### Power

| Function | GPIO | Notes |
|----------|------|-------|
| Power control | 15 | Board power enable (set HIGH on boot) |

### Display Configuration

The display uses the ST7789V controller in 8-bit parallel mode. All driver configuration is done via build flags in `platformio.ini` (no `User_Setup.h` file needed):

- **Resolution:** 170x320 (used in landscape rotation = 320x170)
- **Color depth:** 16-bit RGB565
- **Inversion:** Enabled (required for ST7789V)
- **RGB order:** `TFT_RGB`
- **CGRAM offset:** Enabled
- **Init sequence:** `INIT_SEQUENCE_3` (T-Display-S3 specific)

The full-screen sprite buffer (320x170, 16-bit = ~109 KB) is allocated in PSRAM for flicker-free rendering.

---

## Waveshare ESP32-S3-LCD-1.47

PlatformIO environment: `waveshare147`

| Spec | Value |
|------|-------|
| MCU | ESP32-S3R8 (Xtensa LX7 dual-core, 240 MHz) |
| Flash | 16 MB (QSPI) |
| PSRAM | 8 MB (OPI, integrated in package) |
| Display | 1.47" ST7789 TFT, 172x320 pixels, 262K colors |
| Display Interface | SPI (FSPI, 80 MHz) |
| Interface | USB Type-A (CDC serial, power, programming) |
| Buttons | 1 tactile button (BOOT) + RESET |
| Touch | None |
| WiFi | 802.11 b/g/n (2.4 GHz) |
| Bluetooth | BLE 5.0 (not used in this project) |
| Extras | RGB LED (GPIO 38), SD card slot |

### Pin Configuration

Defined in `include/pin_config.h` when `BOARD_WAVESHARE_147` is set:

#### Display (SPI)

| Function | GPIO | Notes |
|----------|------|-------|
| MOSI | 45 | SPI data out |
| SCLK | 40 | SPI clock |
| CS (Chip Select) | 42 | |
| DC (Data/Command) | 41 | |
| RST (Reset) | 39 | |
| BL (Backlight) | 48 | PWM controlled (channel 0, 5 kHz, 8-bit) |

#### Button

| Button | GPIO | Configuration |
|--------|------|---------------|
| BOOT | 0 | Active LOW, internal pull-up |

RESET button is hardware-only (not readable in software).

#### SD Card

| Function | GPIO |
|----------|------|
| CMD | 15 |
| SCK | 14 |
| D0 | 16 |
| D1 | 18 |
| D2 | 17 |
| D3 | 21 |

SD card is not used by the firmware but pins are reserved.

#### RGB LED

| Function | GPIO | Notes |
|----------|------|-------|
| RGB LED | 38 | Available for status indication (not currently used) |

### Display Configuration

The display uses the ST7789 controller in SPI mode via the FSPI peripheral:

- **Resolution:** 172x320 (used in landscape rotation = 320x172)
- **Color depth:** 16-bit RGB565
- **Inversion:** Enabled (required for ST7789)
- **RGB order:** `TFT_RGB`
- **CGRAM offset:** Enabled
- **SPI frequency:** 80 MHz
- **SPI port:** FSPI (requires `-DUSE_FSPI_PORT` build flag)

The full-screen sprite buffer (320x172, 16-bit = ~110 KB) is allocated in PSRAM for flicker-free rendering.

### Custom Board Definition

The Waveshare ESP32-S3-LCD-1.47 does not have a built-in PlatformIO board definition. A custom JSON is provided at `boards/waveshare_esp32s3_lcd147.json` with the correct flash size (16 MB) and PSRAM configuration (8 MB OPI, `memory_type: qio_opi`).

### Flashing Notes

The Waveshare board uses USB Type-A. To enter download mode for first-time flashing:

1. Hold the **BOOT** button
2. Press and release the **RESET** button (while holding BOOT)
3. Release the **BOOT** button

After the first successful flash with USB-CDC enabled, subsequent uploads should auto-enter download mode.

---

## Common Configuration

### Backlight Control

Both boards use PWM-controlled backlights:

| Parameter | Value |
|-----------|-------|
| PWM channel | 0 |
| Frequency | 5000 Hz |
| Resolution | 8-bit (0-255) |
| Full brightness | 255 |
| Dimmed | 40 |
| Auto-dim timeout | 2 minutes |

Any button press restores full brightness.

### Memory Usage

| Region | Allocation |
|--------|------------|
| Flash | 16 MB (partitioned via `default_16MB.csv`) |
| PSRAM | Sprite buffer (~109-110 KB), SSL buffers |
| NVS | WiFi SSID, WiFi password, API key, session key, proxy URL (namespace: `claudemon`) |

### USB Configuration

Both boards use USB-CDC for programming and serial output (no external USB-UART chip):

```ini
build_flags =
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
```

- **Upload baud:** 921600
- **Monitor baud:** 115200
- **Protocol:** esptool

---

## Feature Flags

Board capabilities are exposed as compile-time feature flags in `include/pin_config.h`:

| Flag | T-Display-S3 | Waveshare 1.47 | Effect |
|------|:---:|:---:|--------|
| `HAS_TWO_BUTTONS` | 1 | 0 | Enables prev/next vs next-only navigation |
| `HAS_POWER_PIN` | 1 | 0 | Controls display power enable on init |
| `HAS_TOUCH` | 1 | 0 | Enables CST816S touch handler |
