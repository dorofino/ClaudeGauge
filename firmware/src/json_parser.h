#pragma once

#include <ArduinoJson.h>
#include "data_models.h"

namespace JsonParser {
    bool parseUsageResponse(const String& json, UsageData& out);
    bool parseCostResponse(const String& json, CostData& out,
                           const char* todayPrefix = nullptr);
    bool parseClaudeCodeResponse(const String& json, ClaudeCodeData& out);
}
