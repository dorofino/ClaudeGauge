#pragma once

#include <WiFi.h>

class WiFiManager {
public:
    void connect(const String& ssid, const String& password);
    bool isConnected();
    void reconnectIfNeeded();
    int16_t getRSSI();
    String getIP();

    void setSavedCredentials(const String& ssid, const String& password);

private:
    String   _ssid;
    String   _password;
    uint32_t _lastReconnectAttempt = 0;
    static constexpr uint32_t RECONNECT_INTERVAL = 30000;
};
