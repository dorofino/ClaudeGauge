#include <Arduino.h>
#include "config.h"
#include "pin_config.h"
#include "colors.h"
#include "data_models.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "api_client.h"
#include "claude_ai_client.h"
#include "display_manager.h"
#include "button_handler.h"
#include "ui_renderer.h"
#include "settings_manager.h"
#include "web_server.h"
#if HAS_TOUCH
#include "touch_handler.h"
#endif

// ============================================================
// Device modes
// ============================================================
enum DeviceMode {
    MODE_SETUP,      // AP mode, web portal, waiting for config
    MODE_CONNECTING,  // Connecting to WiFi with saved credentials
    MODE_DASHBOARD    // Normal operation, showing data
};

// ============================================================
// Global instances
// ============================================================
static AppState        state;
static WiFiManager     wifiMgr;
static TimeManager     timeMgr;
static ApiClient       apiClient;
static ClaudeAiClient  claudeAiClient;
static DisplayManager  displayMgr;
static ButtonHandler   buttonHandler;
#if HAS_TOUCH
static TouchHandler    touchHandler;
#endif
static SettingsManager settingsMgr;
static ConfigWebServer webServer;
static DeviceMode      deviceMode = MODE_SETUP;

// ============================================================
// Forward declarations
// ============================================================
void enterSetupMode();
void enterDashboardMode();
void fetchAllData();
void drawCurrentScreen();
void handleNavigation();
void handleAutoRefresh();
void handleBacklightDim();
void updateWiFiState();
uint32_t getCountdownSec();

// ============================================================
// Setup
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ClaudeGauge v1.0 ===");

    // Initialize state
    memset(&state, 0, sizeof(AppState));
    state.current_screen = SCREEN_OVERVIEW;
    state.backlight_level = BACKLIGHT_FULL;
    state.uptime_start = millis();
    state.last_activity = millis();

    // Initialize hardware
    displayMgr.init();
    buttonHandler.init();
#if HAS_TOUCH
    touchHandler.init();
#endif

    // Show splash
    displayMgr.showSplash();
    delay(1500);

    // Initialize NVS settings
    settingsMgr.begin();

    // Check if device has WiFi credentials
    if (!settingsMgr.hasWiFi()) {
        Serial.println("No WiFi credentials. Entering setup mode.");
        enterSetupMode();
    } else {
        Serial.println("Configuration found. Connecting...");
        deviceMode = MODE_CONNECTING;

        String ssid = settingsMgr.getWiFiSSID();
        String pass = settingsMgr.getWiFiPassword();

        displayMgr.showConnecting("Connecting WiFi...");
        wifiMgr.connect(ssid, pass);
        updateWiFiState();

        if (!wifiMgr.isConnected()) {
            Serial.println("WiFi failed. Entering setup mode.");
            enterSetupMode();
            return;
        }

        // Start web server on the station IP (for reconfiguration)
        webServer.begin(&settingsMgr);

        // Sync NTP
        displayMgr.showConnecting("Syncing time...");
        timeMgr.syncNTP();

        // Initialize API client
        if (settingsMgr.hasApiKey()) {
            String apiKey = settingsMgr.getApiKey();
            apiClient.init(apiKey);
        } else {
            Serial.println("No API key configured. Dashboard will show empty data.");
            Serial.printf("Configure API key at http://%s\n", wifiMgr.getIP().c_str());
            state.api_error = true;
            snprintf(state.last_error, ERROR_MSG_LEN, "No API key - set at %s", wifiMgr.getIP().c_str());
        }

        // Initialize Claude.ai client if session key configured
        if (settingsMgr.hasSessionKey()) {
            String proxyUrl = settingsMgr.getProxyUrl();
            if (proxyUrl.length() == 0) {
                proxyUrl = CLAUDEAI_DEFAULT_PROXY_URL;
            }
            claudeAiClient.init(settingsMgr.getSessionKey(), proxyUrl);
            Serial.printf("Claude.ai configured via %s\n", proxyUrl.c_str());
        } else {
            Serial.println("No Claude.ai session key configured");
        }

        // Fetch data after all clients are initialized
        if (settingsMgr.hasApiKey() && timeMgr.isTimeSynced()) {
            displayMgr.showConnecting("Fetching data...");
            fetchAllData();
        }

        enterDashboardMode();
    }
}

// ============================================================
// Enter setup mode: AP + captive portal
// ============================================================
void enterSetupMode() {
    deviceMode = MODE_SETUP;

    // Start AP
    webServer.startAPMode();
    webServer.begin(&settingsMgr);

    // Show setup screen with AP name and IP
    String apName = webServer.getAPName();
    String apIP = WiFi.softAPIP().toString();
    displayMgr.showSetupScreen(apName.c_str(), apIP.c_str());

    Serial.printf("Setup mode: Connect to '%s' and open http://%s\n",
        apName.c_str(), apIP.c_str());
}

// ============================================================
// Enter dashboard mode
// ============================================================
void enterDashboardMode() {
    deviceMode = MODE_DASHBOARD;
    drawCurrentScreen();
    Serial.println("Dashboard mode active.");
}

// ============================================================
// Main loop
// ============================================================
void loop() {
    // Always handle web server
    webServer.handleClient();

    // Check if web config changed and device should reboot
    if (webServer.shouldReboot()) {
        Serial.println("Settings saved. Rebooting...");
        displayMgr.showConnecting("Restarting...");
        delay(REBOOT_DELAY_MS);
        ESP.restart();
    }

    // Poll buttons and touch
    buttonHandler.update();
#if HAS_TOUCH
    touchHandler.update();
#endif

    if (deviceMode == MODE_SETUP) {
        // In setup mode, just handle web requests
        if (buttonHandler.anyPressed()
#if HAS_TOUCH
            || touchHandler.isTouched()
#endif
        ) {
            state.last_activity = millis();
            displayMgr.setBacklight(BACKLIGHT_FULL);
        }
        handleBacklightDim();
        delay(10);
        return;
    }

    // Dashboard mode
    handleNavigation();
    handleAutoRefresh();
    handleBacklightDim();

    // WiFi reconnection
    if (!wifiMgr.isConnected()) {
        wifiMgr.reconnectIfNeeded();
        updateWiFiState();
    }

    delay(10);
}

// ============================================================
// Data fetching
// ============================================================
void fetchAllData() {
    if (!settingsMgr.hasApiKey()) {
        Serial.println("Skip fetch: No API key configured");
        return;
    }
    if (!wifiMgr.isConnected()) {
        Serial.println("Skip fetch: WiFi disconnected");
        return;
    }
    if (!timeMgr.isTimeSynced()) {
        Serial.println("Skip fetch: NTP not synced");
        timeMgr.syncNTP();
        return;
    }

    state.is_fetching = true;
    strncpy(state.fetch_status, "Initializing...", sizeof(state.fetch_status) - 1);
    drawCurrentScreen();

    Serial.println("--- Fetching data ---");

    String todayStart = timeMgr.todayStartUTC();
    String todayEnd   = timeMgr.todayEndUTC();
    String monthStart = timeMgr.monthStartUTC();
    String todayDate  = timeMgr.todayDateOnly();

    Serial.printf("Date ranges: today=%s to %s, month=%s, date=%s\n",
        todayStart.c_str(), todayEnd.c_str(), monthStart.c_str(), todayDate.c_str());

    bool usageOk = false, costOk = false, codeOk = false;

    // Use static locals to avoid stack overflow (structs are large)
    static UsageData newUsage;
    static CostData newCost;
    static ClaudeCodeData newCode;
    static UsageData newMonthlyUsage;
    static ClaudeCodeData newMonthlyCode;

    // 1. Usage report
    strncpy(state.fetch_status, "Usage report...", sizeof(state.fetch_status) - 1);
    drawCurrentScreen();
    Serial.println("Fetching usage report...");
    memset(&newUsage, 0, sizeof(UsageData));
    if (apiClient.fetchUsageReport(newUsage, todayStart, todayEnd)) {
        state.usage = newUsage;
        usageOk = true;
        Serial.printf("Usage: %llu input, %llu output, %llu cached\n",
            state.usage.today_total.uncached_input,
            state.usage.today_total.output,
            state.usage.today_total.cached_input);
    } else {
        Serial.printf("Usage fetch failed: %s\n", apiClient.getLastError());
    }

    // 2. Cost report - month to date with per-model breakdown
    strncpy(state.fetch_status, "Monthly cost...", sizeof(state.fetch_status) - 1);
    drawCurrentScreen();
    Serial.println("Fetching monthly cost...");
    memset(&newCost, 0, sizeof(CostData));
    if (apiClient.fetchCostReport(newCost, monthStart, todayEnd, todayDate.c_str())) {
        newCost.valid = true;
        newCost.fetched_at = millis();

        // Sort models by cost descending
        for (int i = 0; i < newCost.model_count - 1; i++) {
            for (int j = i + 1; j < newCost.model_count; j++) {
                if (newCost.models[j].cost_usd > newCost.models[i].cost_usd) {
                    ModelCost tmp = newCost.models[i];
                    newCost.models[i] = newCost.models[j];
                    newCost.models[j] = tmp;
                }
            }
        }

        state.cost = newCost;
        costOk = true;
        Serial.printf("Monthly cost: $%.2f, Today: $%.2f, Models: %d\n",
            state.cost.month_usd, state.cost.today_usd, state.cost.model_count);
    } else {
        Serial.printf("Cost (month) failed: %s\n", apiClient.getLastError());
    }

    // 4. Claude Code analytics
    strncpy(state.fetch_status, "Claude Code...", sizeof(state.fetch_status) - 1);
    drawCurrentScreen();
    Serial.println("Fetching Claude Code analytics...");
    memset(&newCode, 0, sizeof(ClaudeCodeData));
    if (apiClient.fetchClaudeCodeReport(newCode, todayDate)) {
        state.code = newCode;
        codeOk = true;
        Serial.printf("CC: %lu sessions, +%ld/-%ld lines\n",
            (unsigned long)state.code.total_sessions,
            (long)state.code.total_lines_added,
            (long)state.code.total_lines_removed);
    } else {
        Serial.printf("CC fetch note: %s\n", apiClient.getLastError());
    }

    // 5. Monthly usage report (by model)
    strncpy(state.fetch_status, "Monthly usage...", sizeof(state.fetch_status) - 1);
    drawCurrentScreen();
    Serial.println("Fetching monthly usage report...");
    memset(&newMonthlyUsage, 0, sizeof(UsageData));
    if (apiClient.fetchUsageReport(newMonthlyUsage, monthStart, todayEnd)) {
        state.monthly_usage = newMonthlyUsage;
        Serial.printf("Monthly usage: %d models, %llu total tokens\n",
            state.monthly_usage.model_count,
            state.monthly_usage.today_total.total());
    } else {
        Serial.printf("Monthly usage failed: %s\n", apiClient.getLastError());
    }

    // 6. Monthly Claude Code analytics
    strncpy(state.fetch_status, "Monthly Code...", sizeof(state.fetch_status) - 1);
    drawCurrentScreen();
    Serial.println("Fetching monthly Claude Code analytics...");
    memset(&newMonthlyCode, 0, sizeof(ClaudeCodeData));
    if (apiClient.fetchClaudeCodeReport(newMonthlyCode, timeMgr.monthStartDateOnly())) {
        state.monthly_code = newMonthlyCode;
        Serial.printf("Monthly CC: %lu sessions, $%.2f cost\n",
            (unsigned long)state.monthly_code.total_sessions,
            state.monthly_code.total_cost);
    } else {
        Serial.printf("Monthly CC note: %s\n", apiClient.getLastError());
    }

    // If cost_report has no data for today, use CC estimated cost as fallback
    if (state.cost.today_usd < 0.01f && state.code.total_cost > 0) {
        state.cost.today_usd = state.code.total_cost;
        Serial.printf("Today cost from CC estimate: $%.2f\n", state.cost.today_usd);
    }

    // Populate per-model cost breakdown from CC analytics
    if (state.cost.model_count == 0 && state.code.model_cost_count > 0) {
        for (int i = 0; i < state.code.model_cost_count && i < MAX_MODELS; i++) {
            state.cost.models[i] = state.code.model_costs[i];
        }
        state.cost.model_count = state.code.model_cost_count;

        // Sort by cost descending
        for (int i = 0; i < state.cost.model_count - 1; i++) {
            for (int j = i + 1; j < state.cost.model_count; j++) {
                if (state.cost.models[j].cost_usd > state.cost.models[i].cost_usd) {
                    ModelCost tmp = state.cost.models[i];
                    state.cost.models[i] = state.cost.models[j];
                    state.cost.models[j] = tmp;
                }
            }
        }
        Serial.printf("Cost breakdown from CC: %d models\n", state.cost.model_count);
    }

    // Fetch Claude.ai subscription usage (if session key configured)
    if (settingsMgr.hasSessionKey()) {
        strncpy(state.fetch_status, "Claude.ai...", sizeof(state.fetch_status) - 1);
        drawCurrentScreen();
        Serial.println("Fetching Claude.ai subscription usage...");
        static ClaudeAiUsage newClaudeAi;
        // Copy current org UUID so fetchUsage can reuse it
        strncpy(newClaudeAi.org_uuid, state.claude_ai.org_uuid, sizeof(newClaudeAi.org_uuid) - 1);

        if (claudeAiClient.fetchUsage(newClaudeAi)) {
            state.claude_ai = newClaudeAi;
            Serial.printf("Claude.ai: 5h=%.0f%% 7d=%.0f%%\n",
                state.claude_ai.five_hour.utilization,
                state.claude_ai.seven_day.utilization);
        } else {
            Serial.printf("Claude.ai fetch failed: %s\n", claudeAiClient.getLastError());
            // Keep previous data if available, just update error
            if (!state.claude_ai.valid) {
                strncpy(state.claude_ai.error, claudeAiClient.getLastError(), ERROR_MSG_LEN - 1);
            }
        }
    }

    // Only flag error if primary data (usage + cost) both failed
    if (!usageOk && !costOk) {
        state.api_error = true;
        strncpy(state.last_error, apiClient.getLastError(), ERROR_MSG_LEN - 1);
    } else {
        state.api_error = false;
        state.last_error[0] = '\0';
    }

    state.is_fetching = false;
    state.last_refresh = millis();
    state.next_refresh = state.last_refresh + REFRESH_INTERVAL_MS;

    Serial.println("--- Fetch complete ---");
    drawCurrentScreen();
}

// ============================================================
// Screen drawing
// ============================================================
void drawCurrentScreen() {
    TFT_eSprite& spr = displayMgr.sprite();
    uint32_t countdown = getCountdownSec();

    switch (state.current_screen) {
        case SCREEN_OVERVIEW:
            UIRenderer::drawOverview(spr, state, countdown);
            break;
        case SCREEN_MODELS:
            UIRenderer::drawModels(spr, state, countdown);
            break;
        case SCREEN_MODELS_MONTHLY:
            UIRenderer::drawMonthlyModels(spr, state, countdown);
            break;
        case SCREEN_CODE:
            UIRenderer::drawClaudeCode(spr, state, countdown);
            break;
        case SCREEN_CODE_MONTHLY:
            UIRenderer::drawMonthlyCode(spr, state, countdown);
            break;
        case SCREEN_STATUS:
            UIRenderer::drawStatus(spr, state, countdown);
            break;
        case SCREEN_CLAUDEAI:
            UIRenderer::drawClaudeAi(spr, state, countdown);
            break;
        default:
            break;
    }

    displayMgr.pushSprite();
}

// ============================================================
// Button navigation
// ============================================================
void handleNavigation() {
    if (buttonHandler.isPrevPressed()) {
        state.current_screen = (ScreenID)(
            (state.current_screen + SCREEN_COUNT - 1) % SCREEN_COUNT);
        drawCurrentScreen();
        state.last_activity = millis();
        displayMgr.setBacklight(BACKLIGHT_FULL);
    }

    if (buttonHandler.isNextPressed()) {
        state.current_screen = (ScreenID)(
            (state.current_screen + 1) % SCREEN_COUNT);
        drawCurrentScreen();
        state.last_activity = millis();
        displayMgr.setBacklight(BACKLIGHT_FULL);
    }

    if (buttonHandler.isRefreshPressed()) {
        Serial.println("Force refresh triggered");
        state.last_activity = millis();
        displayMgr.setBacklight(BACKLIGHT_FULL);
        fetchAllData();
    }

#if HAS_TOUCH
    // Touch: double-tap = next screen, single tap = wake backlight
    if (touchHandler.isDoubleTapped()) {
        state.current_screen = (ScreenID)(
            (state.current_screen + 1) % SCREEN_COUNT);
        drawCurrentScreen();
        state.last_activity = millis();
        displayMgr.setBacklight(BACKLIGHT_FULL);
    } else if (touchHandler.isTouched()) {
        state.last_activity = millis();
        displayMgr.setBacklight(BACKLIGHT_FULL);
    }
#endif
}

// ============================================================
// Auto-refresh
// ============================================================
void handleAutoRefresh() {
    uint32_t now = millis();
    if (state.next_refresh > 0 && now >= state.next_refresh) {
        Serial.println("Auto-refresh triggered");
        fetchAllData();
    }

    if (state.last_refresh == 0 && wifiMgr.isConnected() &&
        timeMgr.isTimeSynced() && !state.is_fetching) {
        fetchAllData();
    }

    // Periodic screen redraw for countdown timer
    static uint32_t lastRedraw = 0;
    if (now - lastRedraw >= 1000) {
        lastRedraw = now;
        drawCurrentScreen();
    }
}

// ============================================================
// Backlight auto-dim
// ============================================================
void handleBacklightDim() {
    if (buttonHandler.anyPressed()
#if HAS_TOUCH
        || touchHandler.isTouched()
#endif
    ) {
        state.last_activity = millis();
        displayMgr.setBacklight(BACKLIGHT_FULL);
        return;
    }

    uint32_t elapsed = millis() - state.last_activity;
    if (elapsed >= DIM_TIMEOUT_MS) {
        displayMgr.setBacklight(BACKLIGHT_DIM);
    }
}

// ============================================================
// Helpers
// ============================================================
void updateWiFiState() {
    state.wifi_connected = wifiMgr.isConnected();
    state.wifi_rssi = wifiMgr.getRSSI();
    String ip = wifiMgr.getIP();
    strncpy(state.wifi_ip, ip.c_str(), IP_LEN - 1);
    state.wifi_ip[IP_LEN - 1] = '\0';
}

uint32_t getCountdownSec() {
    if (state.next_refresh == 0) return 0;
    uint32_t now = millis();
    if (now >= state.next_refresh) return 0;
    return (state.next_refresh - now) / 1000;
}
