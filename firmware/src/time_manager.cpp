#include "time_manager.h"
#include "config.h"
#include <time.h>

void TimeManager::syncNTP() {
    configTime(0, 0, NTP_SERVER, "time.google.com", "time.nist.gov");

    Serial.print("Syncing NTP");
    struct tm timeinfo;
    int attempts = 0;
    while (!getLocalTime(&timeinfo, 1000) && attempts < 10) {
        delay(200);
        Serial.print(".");
        attempts++;
    }

    if (attempts >= 10) {
        Serial.println("\nNTP sync failed (will retry later)");
        _synced = false;
        return;
    }

    _synced = true;
    _lastSync = millis();
    Serial.printf("\nNTP synced: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

bool TimeManager::isTimeSynced() {
    return _synced;
}

String TimeManager::todayStartUTC() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "";

    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT00:00:00Z",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    return String(buf);
}

String TimeManager::todayEndUTC() {
    time_t now;
    time(&now);
    now += 86400; // add one day (ending_at is exclusive)
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);

    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT00:00:00Z",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    return String(buf);
}

String TimeManager::monthStartUTC() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "";

    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-01T00:00:00Z",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1);
    return String(buf);
}

String TimeManager::todayDateOnly() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "";

    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    return String(buf);
}

String TimeManager::monthStartDateOnly() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "";

    char buf[12];
    snprintf(buf, sizeof(buf), "%04d-%02d-01",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1);
    return String(buf);
}

String TimeManager::formatTime(time_t t) {
    struct tm timeinfo;
    gmtime_r(&t, &timeinfo);

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(buf);
}

String TimeManager::formatTimeShort() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "--:--";

    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d",
        timeinfo.tm_hour, timeinfo.tm_min);
    return String(buf);
}
