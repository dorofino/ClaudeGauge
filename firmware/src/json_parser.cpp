#include "json_parser.h"
#include <ArduinoJson.h>

bool JsonParser::parseUsageResponse(const String& json, UsageData& out) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("Usage parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray data = doc["data"].as<JsonArray>();
    if (data.isNull()) return false;

    for (JsonObject bucket : data) {
        JsonArray results = bucket["results"].as<JsonArray>();
        if (results.isNull()) continue;

        for (JsonObject r : results) {
            uint64_t uncached = r["uncached_input_tokens"] | (uint64_t)0;
            uint64_t cached   = r["cache_read_input_tokens"] | (uint64_t)0;
            uint64_t output   = r["output_tokens"] | (uint64_t)0;

            // Cache creation tokens (nested structure)
            uint64_t cache_creation = 0;
            JsonObject cc = r["cache_creation"];
            if (!cc.isNull()) {
                cache_creation += (uint64_t)(cc["ephemeral_5m_input_tokens"] | (uint64_t)0);
                cache_creation += (uint64_t)(cc["ephemeral_1h_input_tokens"] | (uint64_t)0);
            }

            // Aggregate totals
            out.today_total.uncached_input += uncached;
            out.today_total.cached_input   += cached;
            out.today_total.cache_creation += cache_creation;
            out.today_total.output         += output;

            // Per-model breakdown
            const char* modelName = r["model"] | "unknown";
            if (modelName[0] != '\0' && out.model_count < MAX_MODELS) {
                // Check if model already exists (from pagination)
                int existingIdx = -1;
                for (int i = 0; i < out.model_count; i++) {
                    if (strncmp(out.models[i].model_name, modelName, MODEL_NAME_LEN) == 0) {
                        existingIdx = i;
                        break;
                    }
                }

                if (existingIdx >= 0) {
                    ModelUsage& m = out.models[existingIdx];
                    m.tokens.uncached_input += uncached;
                    m.tokens.cached_input   += cached;
                    m.tokens.cache_creation += cache_creation;
                    m.tokens.output         += output;
                } else {
                    ModelUsage& m = out.models[out.model_count];
                    strncpy(m.model_name, modelName, MODEL_NAME_LEN - 1);
                    m.model_name[MODEL_NAME_LEN - 1] = '\0';
                    m.tokens.uncached_input = uncached;
                    m.tokens.cached_input   = cached;
                    m.tokens.cache_creation = cache_creation;
                    m.tokens.output         = output;
                    out.model_count++;
                }
            }
        }
    }

    return true;
}

bool JsonParser::parseCostResponse(const String& json, CostData& out,
                                    const char* todayPrefix) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("Cost parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray data = doc["data"].as<JsonArray>();
    if (data.isNull()) return false;

    for (JsonObject bucket : data) {
        bool isToday = false;
        if (todayPrefix) {
            const char* bucketDate = bucket["starting_at"] | "";
            isToday = (strncmp(bucketDate, todayPrefix, strlen(todayPrefix)) == 0);
        }

        JsonArray results = bucket["results"].as<JsonArray>();
        if (results.isNull()) continue;

        for (JsonObject r : results) {
            const char* amtStr = r["amount"] | "0";
            float amtCents = atof(amtStr);
            float amtUsd = amtCents / 100.0f;

            out.month_usd += amtUsd;
            if (isToday) out.today_usd += amtUsd;

            // Per-model accumulation
            const char* modelName = r["model"] | (const char*)nullptr;
            if (modelName && modelName[0] != '\0') {
                int idx = -1;
                for (int i = 0; i < out.model_count; i++) {
                    if (strncmp(out.models[i].model_name, modelName, MODEL_NAME_LEN) == 0) {
                        idx = i;
                        break;
                    }
                }
                if (idx < 0 && out.model_count < MAX_MODELS) {
                    idx = out.model_count;
                    strncpy(out.models[idx].model_name, modelName, MODEL_NAME_LEN - 1);
                    out.models[idx].model_name[MODEL_NAME_LEN - 1] = '\0';
                    out.models[idx].cost_usd = 0;
                    out.model_count++;
                }
                if (idx >= 0) {
                    out.models[idx].cost_usd += amtUsd;
                }
            }
        }
    }

    return true;
}

bool JsonParser::parseClaudeCodeResponse(const String& json, ClaudeCodeData& out) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("CC parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray data = doc["data"].as<JsonArray>();
    if (data.isNull()) return false;

    for (JsonObject record : data) {
        // Extract core metrics
        JsonObject core = record["core_metrics"];
        if (core.isNull()) continue;

        uint16_t sessions = core["num_sessions"] | 0;
        int32_t  linesAdded   = core["lines_of_code"]["added"] | 0;
        int32_t  linesRemoved = core["lines_of_code"]["removed"] | 0;
        uint16_t commits = core["commits_by_claude_code"] | 0;
        uint16_t prs     = core["pull_requests_by_claude_code"] | 0;

        // Aggregate totals
        out.total_sessions     += sessions;
        out.total_lines_added  += linesAdded;
        out.total_lines_removed += linesRemoved;
        out.total_commits      += commits;
        out.total_prs          += prs;

        // Tool actions
        JsonObject tools = record["tool_actions"];
        if (!tools.isNull()) {
            uint16_t ea = tools["edit_tool"]["accepted"] | 0;
            uint16_t er = tools["edit_tool"]["rejected"] | 0;
            uint16_t wa = tools["write_tool"]["accepted"] | 0;
            uint16_t wr = tools["write_tool"]["rejected"] | 0;

            // Also include multi_edit_tool in edit stats
            ea += (uint16_t)(tools["multi_edit_tool"]["accepted"] | 0);
            er += (uint16_t)(tools["multi_edit_tool"]["rejected"] | 0);

            out.total_edit_accepted  += ea;
            out.total_edit_rejected  += er;
            out.total_write_accepted += wa;
            out.total_write_rejected += wr;
        }

        // Model breakdown for cost
        JsonArray models = record["model_breakdown"].as<JsonArray>();
        if (!models.isNull()) {
            for (JsonObject m : models) {
                JsonObject estCost = m["estimated_cost"];
                if (!estCost.isNull()) {
                    float amount = estCost["amount"] | 0.0f;
                    float costUsd = amount / 100.0f;
                    out.total_cost += costUsd;

                    // Store per-model cost
                    const char* modelId = m["model"] | (const char*)nullptr;
                    if (modelId && modelId[0] != '\0' && costUsd > 0) {
                        int idx = -1;
                        for (int i = 0; i < out.model_cost_count; i++) {
                            if (strncmp(out.model_costs[i].model_name, modelId, MODEL_NAME_LEN) == 0) {
                                idx = i;
                                break;
                            }
                        }
                        if (idx < 0 && out.model_cost_count < MAX_MODELS) {
                            idx = out.model_cost_count;
                            strncpy(out.model_costs[idx].model_name, modelId, MODEL_NAME_LEN - 1);
                            out.model_costs[idx].model_name[MODEL_NAME_LEN - 1] = '\0';
                            out.model_costs[idx].cost_usd = 0;
                            out.model_cost_count++;
                        }
                        if (idx >= 0) {
                            out.model_costs[idx].cost_usd += costUsd;
                        }
                    }
                }
            }
        }

        // Store individual user data (up to MAX_CC_USERS)
        if (out.user_count < MAX_CC_USERS) {
            ClaudeCodeUser& user = out.users[out.user_count];
            JsonObject actor = record["actor"];
            if (!actor.isNull()) {
                const char* email = actor["email_address"] | "";
                if (strlen(email) == 0) {
                    const char* keyName = actor["api_key_name"] | "api_key";
                    strncpy(user.email, keyName, EMAIL_LEN - 1);
                } else {
                    strncpy(user.email, email, EMAIL_LEN - 1);
                }
                user.email[EMAIL_LEN - 1] = '\0';
            }

            user.sessions      = sessions;
            user.lines_added   = linesAdded;
            user.lines_removed = linesRemoved;
            user.commits       = commits;
            user.pull_requests = prs;

            if (!tools.isNull()) {
                user.edit_accepted  = tools["edit_tool"]["accepted"] | 0;
                user.edit_rejected  = tools["edit_tool"]["rejected"] | 0;
                user.write_accepted = tools["write_tool"]["accepted"] | 0;
                user.write_rejected = tools["write_tool"]["rejected"] | 0;
            }

            // User-level cost
            float userCost = 0;
            if (!models.isNull()) {
                for (JsonObject m : models) {
                    float amt = m["estimated_cost"]["amount"] | 0.0f;
                    userCost += amt / 100.0f;
                }
            }
            user.estimated_cost = userCost;

            out.user_count++;
        }
    }

    return true;
}
