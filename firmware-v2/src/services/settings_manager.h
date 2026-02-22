#pragma once

#include <Arduino.h>

class SettingsManager {
public:
    void begin();

    // WiFi
    String getWiFiSSID();
    String getWiFiPassword();
    void setWiFi(const String& ssid, const String& password);

    // API Key
    String getApiKey();
    void setApiKey(const String& key);

    // Claude.ai Session Key + Proxy
    String getSessionKey();
    void setSessionKey(const String& key);
    bool hasSessionKey();
    String getProxyUrl();
    void setProxyUrl(const String& url);
    bool hasProxy();

    // Check configuration state
    bool hasWiFi();        // WiFi credentials saved
    bool hasApiKey();      // API key saved
    bool isConfigured();   // Both WiFi and API key saved

    // Display
    bool getFlipScreen();
    void setFlipScreen(bool flip);

    // Clear all settings
    void clear();

private:
    static constexpr const char* NVS_NAMESPACE = "claudemon";
    static constexpr const char* KEY_SSID      = "wifi_ssid";
    static constexpr const char* KEY_PASS      = "wifi_pass";
    static constexpr const char* KEY_API       = "api_key";
    static constexpr const char* KEY_SESSION   = "session_key";
    static constexpr const char* KEY_PROXY     = "proxy_host";   // Legacy
    static constexpr const char* KEY_PROXY_URL = "proxy_url";
    static constexpr const char* KEY_FLIP      = "flip_screen";
};
