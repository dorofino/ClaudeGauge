#pragma once

#include <Arduino.h>
#include <vector>

struct WiFiNetwork {
    String ssid;
    String password;
};

class SettingsManager {
public:
    void begin();

    // WiFi — single-network accessors kept for status display (return the
    // first/most-recently-connected saved network)
    String getWiFiSSID();
    String getWiFiPassword();
    void setWiFi(const String& ssid, const String& password);

    // WiFi — multi-network storage (up to WIFI_MAX_NETWORKS)
    std::vector<WiFiNetwork> getWiFiNetworks();
    // Adds a network, or updates the password if the SSID is already saved.
    // Evicts the oldest entry when the list is full.
    void addWiFiNetwork(const String& ssid, const String& password);
    void removeWiFiNetwork(const String& ssid);

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
    static constexpr const char* KEY_SSID      = "wifi_ssid";   // Legacy single-network
    static constexpr const char* KEY_PASS      = "wifi_pass";   // Legacy single-network
    static constexpr const char* KEY_NETS      = "wifi_nets";   // JSON array of {s,p}
    static constexpr const char* KEY_API       = "api_key";
    static constexpr const char* KEY_SESSION   = "session_key";
    static constexpr const char* KEY_PROXY     = "proxy_host";   // Legacy
    static constexpr const char* KEY_PROXY_URL = "proxy_url";
    static constexpr const char* KEY_FLIP      = "flip_screen";
};
