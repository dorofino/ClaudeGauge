#pragma once

#include <Arduino.h>

class TimeManager {
public:
    void syncNTP();
    bool isTimeSynced();

    // Generate ISO8601 strings for API queries
    String todayStartUTC();     // "2026-02-21T00:00:00Z"
    String todayEndUTC();       // "2026-02-22T00:00:00Z"
    String monthStartUTC();     // "2026-02-01T00:00:00Z"
    String todayDateOnly();     // "2026-02-21" (for Claude Code API)
    String monthStartDateOnly(); // "2026-02-01" (for monthly Claude Code)
    String formatTime(time_t t);
    String formatTimeShort();   // "HH:MM" current time

private:
    bool     _synced = false;
    uint32_t _lastSync = 0;
};
