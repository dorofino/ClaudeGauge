#pragma once

#include <WiFiClientSecure.h>
#include "app.h"

class ClaudeAiClient {
public:
    void init(const String& sessionKey, const String& proxyUrl);

    // Fetch org UUID (only needed once, then cached in ClaudeAiUsage)
    bool fetchOrganization(char* orgUuid, size_t len);

    // Fetch rate limits + extra usage into output struct
    bool fetchUsage(ClaudeAiUsage& out);

    const char* getLastError() const { return _lastError; }

    // Epoch parsed from the most recent response's "Date" header, or 0 if
    // none was seen yet. Lets the caller set the system clock from an
    // HTTPS response when NTP (UDP/123) is blocked by the network but
    // HTTPS (TCP/443) isn't — e.g. a corporate/guest WiFi.
    time_t getLastServerTime() const { return _lastServerTime; }

private:
    WiFiClientSecure _secureClient;
    String   _sessionKey;
    String   _proxyBaseUrl;
    bool     _useHttps;
    char     _lastError[ERROR_MSG_LEN];
    time_t   _lastServerTime = 0;

    bool doGet(const String& path, String& response);
    void setError(const char* fmt, ...);
};
