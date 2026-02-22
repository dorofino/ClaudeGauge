#pragma once

#include <WiFiClientSecure.h>
#include "data_models.h"

class ApiClient {
public:
    void init(const String& apiKey);

    bool fetchUsageReport(UsageData& out, const String& startAt, const String& endAt);
    bool fetchCostReport(CostData& out, const String& startAt, const String& endAt,
                         const char* todayPrefix = nullptr);
    bool fetchClaudeCodeReport(ClaudeCodeData& out, const String& date);

    const char* getLastError() const { return _lastError; }

private:
    WiFiClientSecure _secureClient;
    String           _apiKey;
    char _lastError[ERROR_MSG_LEN];

    bool doGet(const String& url, String& response);
    void setError(const char* fmt, ...);
};
