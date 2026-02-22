#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>

void WiFiManager::setSavedCredentials(const String& ssid, const String& password) {
    _ssid = ssid;
    _password = password;
}

void WiFiManager::connect(const String& ssid, const String& password) {
    _ssid = ssid;
    _password = password;

    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid.c_str(), _password.c_str());

    Serial.printf("Connecting to WiFi '%s'", _ssid.c_str());

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT_MS) {
            Serial.println("\nWiFi connection timeout!");
            return;
        }
        delay(250);
        Serial.print(".");
    }

    Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::reconnectIfNeeded() {
    if (isConnected()) return;
    if (_ssid.length() == 0) return;

    uint32_t now = millis();
    if (now - _lastReconnectAttempt < RECONNECT_INTERVAL) return;

    _lastReconnectAttempt = now;
    Serial.println("WiFi reconnecting...");
    WiFi.disconnect();
    WiFi.begin(_ssid.c_str(), _password.c_str());
}

int16_t WiFiManager::getRSSI() {
    if (!isConnected()) return -100;
    return WiFi.RSSI();
}

String WiFiManager::getIP() {
    if (!isConnected()) return "0.0.0.0";
    return WiFi.localIP().toString();
}
