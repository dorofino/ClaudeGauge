#include "api_client.h"
#include "config.h"
#include "json_parser.h"
#include <HTTPClient.h>
#include <stdarg.h>

void ApiClient::init(const String& apiKey) {
    _apiKey = apiKey;
    // Skip certificate validation — TLS encryption still active,
    // just no server identity verification (acceptable for home device)
    _secureClient.setInsecure();
    _secureClient.setTimeout(15);  // 15 second timeout
    memset(_lastError, 0, sizeof(_lastError));
}

void ApiClient::setError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(_lastError, ERROR_MSG_LEN, fmt, args);
    va_end(args);
    Serial.printf("API Error: %s\n", _lastError);
}

bool ApiClient::doGet(const String& url, String& response) {
    Serial.printf("[API] GET %s\n", url.c_str());

    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(15000);
    http.setReuse(false);

    if (!http.begin(_secureClient, url)) {
        setError("HTTP begin failed");
        return false;
    }

    http.addHeader("x-api-key", _apiKey);
    http.addHeader("anthropic-version", API_VERSION);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.GET();
    Serial.printf("[API] HTTP %d\n", httpCode);

    if (httpCode != 200) {
        String errBody = http.getString();
        Serial.printf("[API] Error body: %.500s\n", errBody.c_str());
        if (httpCode == 401) setError("Invalid API key (401)");
        else if (httpCode == 403) setError("Admin key required (403)");
        else if (httpCode == 429) setError("Rate limited (429)");
        else setError("HTTP %d", httpCode);
        http.end();
        return false;
    }

    response = http.getString();
    http.end();

    if (response.length() == 0) {
        setError("Empty response");
        return false;
    }

    // Debug: print first 500 chars of response
    Serial.printf("[API] Response (%d bytes): %.500s\n", response.length(), response.c_str());

    return true;
}

bool ApiClient::fetchUsageReport(UsageData& out, const String& startAt, const String& endAt) {
    memset(&out, 0, sizeof(UsageData));

    String url = String("https://") + API_HOST +
        "/v1/organizations/usage_report/messages"
        "?starting_at=" + startAt +
        "&ending_at=" + endAt +
        "&bucket_width=1d"
        "&group_by[]=model";

    String nextPage;
    bool firstPage = true;

    do {
        String requestUrl = url;
        if (nextPage.length() > 0) {
            requestUrl += "&page=" + nextPage;
        }

        String response;
        if (!doGet(requestUrl, response)) {
            return false;
        }

        if (!JsonParser::parseUsageResponse(response, out)) {
            setError("Usage JSON parse fail");
            return false;
        }

        // Check for pagination
        JsonDocument pageDoc;
        DeserializationError err = deserializeJson(pageDoc, response);
        if (err) break;

        bool hasMore = pageDoc["has_more"] | false;
        if (hasMore) {
            const char* np = pageDoc["next_page"];
            nextPage = np ? String(np) : "";
        } else {
            nextPage = "";
        }

        firstPage = false;
    } while (nextPage.length() > 0);

    out.valid = true;
    out.fetched_at = millis();
    return true;
}

bool ApiClient::fetchCostReport(CostData& out, const String& startAt, const String& endAt,
                                 const char* todayPrefix) {
    String url = String("https://") + API_HOST +
        "/v1/organizations/cost_report"
        "?starting_at=" + startAt +
        "&bucket_width=1d";

    if (endAt.length() > 0) {
        url += "&ending_at=" + endAt;
    }

    String nextPage;

    do {
        String requestUrl = url;
        if (nextPage.length() > 0) {
            requestUrl += "&page=" + nextPage;
        }

        String response;
        if (!doGet(requestUrl, response)) {
            return false;
        }

        if (!JsonParser::parseCostResponse(response, out, todayPrefix)) {
            setError("Cost JSON parse fail");
            return false;
        }

        // Check for pagination
        JsonDocument pageDoc;
        DeserializationError err = deserializeJson(pageDoc, response);
        if (err) break;

        bool hasMore = pageDoc["has_more"] | false;
        if (hasMore) {
            const char* np = pageDoc["next_page"];
            nextPage = np ? String(np) : "";
        } else {
            nextPage = "";
        }
    } while (nextPage.length() > 0);

    return true;
}

bool ApiClient::fetchClaudeCodeReport(ClaudeCodeData& out, const String& date) {
    memset(&out, 0, sizeof(ClaudeCodeData));

    String url = String("https://") + API_HOST +
        "/v1/organizations/usage_report/claude_code"
        "?starting_at=" + date +
        "&limit=100";

    String nextPage;

    do {
        String requestUrl = url;
        if (nextPage.length() > 0) {
            requestUrl += "&page=" + nextPage;
        }

        String response;
        if (!doGet(requestUrl, response)) {
            return false;
        }

        if (!JsonParser::parseClaudeCodeResponse(response, out)) {
            setError("CC JSON parse fail");
            return false;
        }

        // Check for pagination
        JsonDocument pageDoc;
        DeserializationError err = deserializeJson(pageDoc, response);
        if (err) break;

        bool hasMore = pageDoc["has_more"] | false;
        if (hasMore) {
            const char* np = pageDoc["next_page"];
            nextPage = np ? String(np) : "";
        } else {
            nextPage = "";
        }
    } while (nextPage.length() > 0);

    out.valid = true;
    out.fetched_at = millis();
    return true;
}
