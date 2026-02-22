#include "settings_manager.h"
#include "config.h"
#include <Preferences.h>

static Preferences prefs;

void SettingsManager::begin() {
    prefs.begin(NVS_NAMESPACE, false);

    // One-time cleanup: remove stale proxy URLs from NVS.
    // Catches: legacy http:// local proxy IPs, old workers.dev URLs,
    // and URLs matching the compiled default (so firmware updates take effect).
    String stored = prefs.getString(KEY_PROXY_URL, "");
    if (stored.length() > 0) {
        if (!stored.startsWith("https://") ||
            stored == CLAUDEAI_DEFAULT_PROXY_URL ||
            stored.indexOf("workers.dev") >= 0) {
            Serial.printf("[Settings] Removing stale proxy_url: %s\n", stored.c_str());
            prefs.remove(KEY_PROXY_URL);
        }
    }

    // Also clean up legacy proxy_host key if present
    if (prefs.getString(KEY_PROXY, "").length() > 0) {
        prefs.remove(KEY_PROXY);
        Serial.println("[Settings] Removed legacy proxy_host");
    }
}

String SettingsManager::getWiFiSSID() {
    return prefs.getString(KEY_SSID, "");
}

String SettingsManager::getWiFiPassword() {
    return prefs.getString(KEY_PASS, "");
}

void SettingsManager::setWiFi(const String& ssid, const String& password) {
    prefs.putString(KEY_SSID, ssid);
    prefs.putString(KEY_PASS, password);
}

String SettingsManager::getApiKey() {
    return prefs.getString(KEY_API, "");
}

void SettingsManager::setApiKey(const String& key) {
    prefs.putString(KEY_API, key);
}

String SettingsManager::getSessionKey() {
    return prefs.getString(KEY_SESSION, "");
}

void SettingsManager::setSessionKey(const String& key) {
    prefs.putString(KEY_SESSION, key);
}

bool SettingsManager::hasSessionKey() {
    return getSessionKey().length() > 0;
}

String SettingsManager::getProxyUrl() {
    // Check for stored proxy_url (user-configured)
    String url = prefs.getString(KEY_PROXY_URL, "");
    if (url.length() > 0) return url;

    // Clean up legacy proxy_host if present
    if (prefs.getString(KEY_PROXY, "").length() > 0) {
        prefs.remove(KEY_PROXY);
        Serial.println("[Settings] Removed legacy proxy_host");
    }

    // Return empty — caller falls through to CLAUDEAI_DEFAULT_PROXY_URL
    return "";
}

void SettingsManager::setProxyUrl(const String& url) {
    prefs.putString(KEY_PROXY_URL, url);
    prefs.remove(KEY_PROXY);  // Clean up legacy key
}

bool SettingsManager::hasProxy() {
    return getProxyUrl().length() > 0;
}

bool SettingsManager::hasWiFi() {
    return getWiFiSSID().length() > 0;
}

bool SettingsManager::hasApiKey() {
    return getApiKey().length() > 0;
}

bool SettingsManager::isConfigured() {
    return hasWiFi() && hasApiKey();
}

void SettingsManager::clear() {
    prefs.clear();
}
