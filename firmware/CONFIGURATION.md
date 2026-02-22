# Configuration Guide

## First-Time Setup

When the device boots without saved credentials, it enters **Setup Mode** automatically.

### Step 1: Connect to the Device

The device creates a WiFi access point named **`ClaudeGauge-XXXX`** (where XXXX is derived from the board's MAC address). The AP name and IP are shown on the display.

Connect your phone or laptop to this network.

### Step 2: Open the Configuration Portal

A captive portal should open automatically. If it doesn't, open a browser and navigate to:

```
http://192.168.4.1
```

The portal displays the current configuration status and provides forms for WiFi and API key setup.

### Step 3: Configure WiFi

Enter your WiFi network name (SSID) and password, then click **Save WiFi & Restart**.

### Step 4: Configure API Key

Enter your Anthropic API key and click **Save API Key & Restart**.

**Recommended:** Use an admin key (`sk-ant-admin01-...`) for full access to usage reports, cost data, and Claude Code analytics.

A regular API key (`sk-ant-api...`) provides limited functionality — usage data may be restricted and Claude Code analytics will not be available.

### Step 5: Configure Claude.ai Session (Optional)

To track Claude.ai subscription rate limits (5-hour and 7-day usage), you need a session key from claude.ai.

**Recommended:** Install the [Chrome extension](../extension/) for one-click setup:

1. Install the extension from the Chrome Web Store
2. Log into [claude.ai](https://claude.ai) in your browser
3. Open the device's config page in your browser
4. Click the **Auto-fill from Claude.ai** button that appears on the page

**Manual setup:** Open DevTools (F12) on claude.ai, go to Application → Cookies, and copy the `sessionKey` value. Paste it into the Session Key field on the config portal.

The proxy URL field defaults to the built-in cloud proxy. Only change this if you are self-hosting the proxy.

### Step 6: Device Reboots

After saving, the device reboots automatically (2-second delay). It will:

1. Connect to the configured WiFi network
2. Sync time via NTP
3. Fetch initial data from the Anthropic API
4. Enter dashboard mode

If WiFi connection fails, the device falls back to Setup Mode.

## Reconfiguration

Once the device is running in dashboard mode, you can reconfigure it **without a factory reset** by accessing the web portal at the device's local IP address.

### Finding the Device IP

The IP address is displayed on the **System Status** screen (screen 6). Navigate there using the buttons:

- **T-Display-S3:** Left/right buttons to navigate between screens
- **Waveshare 1.47:** BOOT button short press to cycle forward through screens

### Accessing the Portal

Open a browser on the same WiFi network and navigate to:

```
http://<device-ip>
```

For example: `http://192.168.1.36`

The portal shows the current status and allows you to update WiFi credentials or the API key independently.

## Factory Reset

To clear all saved settings:

1. Open the web portal (either via AP mode or station IP)
2. Scroll to the bottom and click **Reset All Settings**
3. Confirm the action
4. The device clears NVS storage and reboots into Setup Mode

Alternatively, re-flash the firmware to start fresh.

## Web Portal Endpoints

The built-in web server runs on port 80 and provides:

| URL | Method | Description |
|-----|--------|-------------|
| `/` | GET | Configuration form with current status |
| `/save-wifi` | POST | Save WiFi SSID and password |
| `/save-apikey` | POST | Save Anthropic API key |
| `/save-session` | POST | Save Claude.ai session key and proxy URL |
| `/status` | GET | JSON status object |
| `/reset` | POST | Clear all settings and reboot |

### Status JSON

`GET /status` returns:

```json
{
  "configured": true,
  "wifi_ssid": "MyNetwork",
  "has_api_key": true,
  "wifi_connected": true,
  "ip": "192.168.1.36",
  "rssi": -58,
  "heap": 205824
}
```

## NVS Storage

Credentials are stored in the ESP32's Non-Volatile Storage (NVS) and persist across reboots and power cycles. They are **not** included in the firmware binary.

| Namespace | Key | Content |
|-----------|-----|---------|
| `claudemon` | `wifi_ssid` | WiFi network name |
| `claudemon` | `wifi_pass` | WiFi password |
| `claudemon` | `api_key` | Anthropic API key |
| `claudemon` | `session_key` | Claude.ai session key |
| `claudemon` | `proxy_url` | Cloud proxy URL for Claude.ai requests |

## Getting an Anthropic API Key

1. Go to the [Anthropic Console](https://console.anthropic.com/)
2. Navigate to **Settings** → **API Keys**
3. Create a new key
4. For full dashboard features, use an **Admin key** (requires organization admin access)

## Network Requirements

The device needs:

- WiFi access (2.4 GHz, 802.11 b/g/n)
- Outbound HTTPS (port 443) to `api.anthropic.com`
- Outbound HTTPS (port 443) to the cloud proxy URL (for Claude.ai subscription data)
- Outbound NTP (UDP port 123) to `pool.ntp.org`, `time.google.com`, or `time.nist.gov`

No inbound ports need to be opened. The web portal is only accessible from the local network.

## Firmware Upload

### Using the Upload Script (Windows)

The interactive PowerShell script handles board selection and upload:

```powershell
.\scripts\upload.ps1
```

It prompts you to choose the target board and runs the appropriate PlatformIO upload command.

### Manual Upload

```bash
# LILYGO T-Display-S3
pio run -e tdisplays3 -t upload

# Waveshare ESP32-S3-LCD-1.47
pio run -e waveshare147 -t upload
```

### Waveshare First-Time Flashing

The Waveshare board may need to be manually put into download mode for the first flash:

1. Hold the **BOOT** button
2. Press and release the **RESET** button (while holding BOOT)
3. Release the **BOOT** button
4. Run the upload command

After the first successful flash with USB-CDC enabled, subsequent uploads should auto-enter download mode.
