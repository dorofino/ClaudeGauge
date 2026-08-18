#include "settings_manager.h"
#include "config.h"
#include <Preferences.h>
#include <ArduinoJson.h>

static Preferences prefs;

void SettingsManager::begin() {
    prefs.begin(NVS_NAMESPACE, false);

    // One-time migration: fold the legacy single-network credentials into
    // the multi-network list, then drop them.
    String legacySsid = prefs.getString(KEY_SSID, "");
    if (legacySsid.length() > 0) {
        addWiFiNetwork(legacySsid, prefs.getString(KEY_PASS, ""));
        prefs.remove(KEY_SSID);
        prefs.remove(KEY_PASS);
        Serial.println("[Settings] Migrated legacy WiFi credentials");
    }

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
    auto nets = getWiFiNetworks();
    return nets.empty() ? "" : nets.front().ssid;
}

String SettingsManager::getWiFiPassword() {
    auto nets = getWiFiNetworks();
    return nets.empty() ? "" : nets.front().password;
}

void SettingsManager::setWiFi(const String& ssid, const String& password) {
    addWiFiNetwork(ssid, password);
}

std::vector<WiFiNetwork> SettingsManager::getWiFiNetworks() {
    std::vector<WiFiNetwork> nets;
    String raw = prefs.getString(KEY_NETS, "");
    if (raw.length() == 0) return nets;

    JsonDocument doc;
    if (deserializeJson(doc, raw) != DeserializationError::Ok) return nets;

    for (JsonObject o : doc.as<JsonArray>()) {
        WiFiNetwork n;
        n.ssid = o["s"].as<String>();
        n.password = o["p"].as<String>();
        if (n.ssid.length() > 0) nets.push_back(n);
    }
    return nets;
}

static void saveWiFiNetworks(Preferences& p, const std::vector<WiFiNetwork>& nets) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (auto& n : nets) {
        JsonObject o = arr.add<JsonObject>();
        o["s"] = n.ssid;
        o["p"] = n.password;
    }
    String out;
    serializeJson(doc, out);
    p.putString("wifi_nets", out);
}

void SettingsManager::addWiFiNetwork(const String& ssid, const String& password) {
    if (ssid.length() == 0) return;

    std::vector<WiFiNetwork> nets = getWiFiNetworks();

    // Update in place if this SSID is already saved, moving it to the front
    // (most-recently-used) so getWiFiSSID()/getWiFiPassword() reflect it.
    for (size_t i = 0; i < nets.size(); i++) {
        if (nets[i].ssid == ssid) {
            nets.erase(nets.begin() + i);
            break;
        }
    }
    nets.insert(nets.begin(), {ssid, password});

    while (nets.size() > WIFI_MAX_NETWORKS) {
        nets.pop_back();
    }

    saveWiFiNetworks(prefs, nets);
}

void SettingsManager::removeWiFiNetwork(const String& ssid) {
    std::vector<WiFiNetwork> nets = getWiFiNetworks();
    for (size_t i = 0; i < nets.size(); i++) {
        if (nets[i].ssid == ssid) {
            nets.erase(nets.begin() + i);
            saveWiFiNetworks(prefs, nets);
            return;
        }
    }
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

bool SettingsManager::getFlipScreen() {
    return prefs.getUChar(KEY_FLIP, 0) != 0;
}

void SettingsManager::setFlipScreen(bool flip) {
    prefs.putUChar(KEY_FLIP, flip ? 1 : 0);
}

void SettingsManager::clear() {
    prefs.clear();
}
