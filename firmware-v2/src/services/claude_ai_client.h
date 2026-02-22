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

private:
    WiFiClientSecure _secureClient;
    String   _sessionKey;
    String   _proxyBaseUrl;
    bool     _useHttps;
    char     _lastError[ERROR_MSG_LEN];

    bool doGet(const String& path, String& response);
    void setError(const char* fmt, ...);
};
