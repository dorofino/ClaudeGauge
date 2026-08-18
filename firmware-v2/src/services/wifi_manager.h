#pragma once

#include <WiFi.h>
#include <WiFiMulti.h>
#include <vector>
#include "settings_manager.h"

class WiFiManager {
public:
    // Legacy single-network connect (still used by callers that only have
    // one SSID/password on hand, e.g. right after the setup form).
    void connect(const String& ssid, const String& password);

    // Registers every saved network with WiFiMulti and connects to
    // whichever is reachable first.
    void connectAny(const std::vector<WiFiNetwork>& networks);

    bool isConnected();
    void reconnectIfNeeded();
    int16_t getRSSI();
    String getIP();

    void setSavedCredentials(const String& ssid, const String& password);

private:
    String   _ssid;
    String   _password;
    WiFiMulti _wifiMulti;
    bool      _multiMode = false;
    uint32_t _lastReconnectAttempt = 0;
    static constexpr uint32_t RECONNECT_INTERVAL = 30000;
};
