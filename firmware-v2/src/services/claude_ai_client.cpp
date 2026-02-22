#include "claude_ai_client.h"
#include "config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <stdarg.h>

void ClaudeAiClient::init(const String& sessionKey, const String& proxyUrl) {
    _sessionKey = sessionKey;
    _proxyBaseUrl = proxyUrl;
    // Remove trailing slash if present
    if (_proxyBaseUrl.endsWith("/")) {
        _proxyBaseUrl.remove(_proxyBaseUrl.length() - 1);
    }
    _useHttps = _proxyBaseUrl.startsWith("https://");
    memset(_lastError, 0, sizeof(_lastError));

    if (_useHttps) {
        _secureClient.setInsecure();
        _secureClient.setTimeout(15);
    }

    Serial.printf("[ClaudeAI] Proxy URL: %s (HTTPS: %s)\n",
                  _proxyBaseUrl.c_str(), _useHttps ? "yes" : "no");
}

void ClaudeAiClient::setError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(_lastError, ERROR_MSG_LEN, fmt, args);
    va_end(args);
    Serial.printf("[ClaudeAI] Error: %s\n", _lastError);
}

bool ClaudeAiClient::doGet(const String& path, String& response) {
    String fullUrl = _proxyBaseUrl + path;
    Serial.printf("[ClaudeAI] GET %s\n", fullUrl.c_str());

    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(15000);
    http.setReuse(false);

    bool begun;
    if (_useHttps) {
        begun = http.begin(_secureClient, fullUrl);
    } else {
        WiFiClient plainClient;
        begun = http.begin(plainClient, fullUrl);
    }

    if (!begun) {
        setError("HTTP begin failed");
        return false;
    }

    // Session key sent as header — proxy forwards it as cookie to claude.ai
    http.addHeader("X-Session-Key", _sessionKey);

    int httpCode = http.GET();
    Serial.printf("[ClaudeAI] HTTP %d\n", httpCode);

    if (httpCode <= 0) {
        setError("Proxy unreachable (%d)", httpCode);
        http.end();
        return false;
    }

    if (httpCode != 200) {
        String errBody = http.getString();
        Serial.printf("[ClaudeAI] Error: %.300s\n", errBody.c_str());

        if (httpCode == 401)      setError("Session expired (401)");
        else if (httpCode == 403) setError("Forbidden (403)");
        else if (httpCode == 429) setError("Rate limited (429)");
        else if (httpCode == 502) setError("Proxy error (502)");
        else                      setError("HTTP %d", httpCode);
        http.end();
        return false;
    }

    response = http.getString();
    http.end();

    if (response.length() == 0) {
        setError("Empty response");
        return false;
    }

    Serial.printf("[ClaudeAI] Response (%d bytes): %.200s\n",
                  response.length(), response.c_str());
    return true;
}

bool ClaudeAiClient::fetchOrganization(char* orgUuid, size_t len) {
    String response;

    if (!doGet("/api/organizations", response)) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        setError("JSON parse: %s", err.c_str());
        return false;
    }

    JsonArray orgs = doc.as<JsonArray>();
    if (orgs.size() == 0) {
        setError("No organizations");
        return false;
    }

    const char* uuid = orgs[0]["uuid"] | "";
    if (strlen(uuid) == 0) {
        setError("No org UUID");
        return false;
    }

    strncpy(orgUuid, uuid, len - 1);
    orgUuid[len - 1] = '\0';

    Serial.printf("[ClaudeAI] Org UUID: %s\n", orgUuid);
    return true;
}

// Parse a single limit object from the usage response
static void parseLimit(JsonObject obj, ClaudeAiLimit& limit) {
    if (obj.isNull()) {
        limit.present = false;
        limit.utilization = 0;
        limit.resets_at = 0;
        return;
    }
    limit.present = true;
    limit.utilization = obj["utilization"] | 0.0f;

    // Parse ISO 8601 timestamp to epoch seconds
    const char* resetStr = obj["resets_at"] | "";
    if (strlen(resetStr) > 0) {
        struct tm tm = {};
        sscanf(resetStr, "%d-%d-%dT%d:%d:%d",
               &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
               &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        limit.resets_at = mktime(&tm);  // UTC
    }
}

bool ClaudeAiClient::fetchUsage(ClaudeAiUsage& out) {
    // Preserve org UUID from caller before clearing
    char savedUuid[48] = {};
    strncpy(savedUuid, out.org_uuid, sizeof(savedUuid) - 1);
    memset(&out, 0, sizeof(ClaudeAiUsage));
    strncpy(out.org_uuid, savedUuid, sizeof(out.org_uuid) - 1);

    // Step 1: Get org UUID if not cached
    if (strlen(out.org_uuid) == 0) {
        if (!fetchOrganization(out.org_uuid, sizeof(out.org_uuid))) {
            strncpy(out.error, _lastError, ERROR_MSG_LEN - 1);
            return false;
        }
    }

    // Step 2: Fetch usage/rate limits
    {
        String path = String("/api/organizations/") + out.org_uuid + "/usage";
        String response;

        if (!doGet(path, response)) {
            strncpy(out.error, _lastError, ERROR_MSG_LEN - 1);
            return false;
        }

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, response);
        if (err) {
            setError("Usage JSON: %s", err.c_str());
            strncpy(out.error, _lastError, ERROR_MSG_LEN - 1);
            return false;
        }

        parseLimit(doc["five_hour"], out.five_hour);
        parseLimit(doc["seven_day"], out.seven_day);
        parseLimit(doc["seven_day_opus"], out.seven_day_opus);
        parseLimit(doc["seven_day_sonnet"], out.seven_day_sonnet);
    }

    // Step 3: Fetch extra usage (optional, failure is non-fatal)
    {
        String path = String("/api/organizations/") + out.org_uuid + "/overage_spend_limit";
        String response;

        if (doGet(path, response)) {
            JsonDocument doc;
            if (!deserializeJson(doc, response)) {
                out.extra_enabled = doc["is_enabled"] | false;
                if (out.extra_enabled) {
                    // used_credits is in cents
                    out.extra_used_cents  = doc["used_credits"] | 0;
                    // monthly_credit_limit can be null (unlimited)
                    if (!doc["monthly_credit_limit"].isNull()) {
                        out.extra_limit_cents = doc["monthly_credit_limit"] | 0;
                    }
                }
            }
        }
    }

    out.valid = true;
    out.fetched_at = millis();
    out.error[0] = '\0';

    Serial.printf("[ClaudeAI] 5h: %.0f%%, 7d: %.0f%%, opus: %.0f%%, sonnet: %.0f%%\n",
                  out.five_hour.utilization, out.seven_day.utilization,
                  out.seven_day_opus.utilization, out.seven_day_sonnet.utilization);
    if (out.extra_enabled) {
        Serial.printf("[ClaudeAI] Extra: %u cents used\n", out.extra_used_cents);
    }

    return true;
}
