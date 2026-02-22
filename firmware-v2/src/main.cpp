// ============================================================
// ClaudeGauge v2 — Built on lcars-esp32 engine
// Phase 1: Claude.ai subscription tracking
// ============================================================

#include <lcars.h>
#include "app.h"
#include "config.h"
#include "pin_config.h"

// Services
#include "services/wifi_manager.h"
#include "services/settings_manager.h"
#include "services/web_server.h"
#include "services/time_manager.h"
#include "services/claude_ai_client.h"

// Input
#include "input/button_handler.h"
#if HAS_TOUCH
#include "input/touch_handler.h"
#endif

// Screens
#include "screens/claudeai_screen.h"
#include "screens/status_screen.h"
#include "screens/setup_screen.h"

// ============================================================
// Device modes
// ============================================================
enum DeviceMode {
    MODE_SETUP,
    MODE_CONNECTING,
    MODE_DASHBOARD
};

// ============================================================
// Globals
// ============================================================
static TFT_eSPI       tft;
static LcarsEngine     engine;
static AppState        state;

// Services
static WiFiManager     wifiMgr;
static SettingsManager settingsMgr;
static ConfigWebServer webServer;
static TimeManager     timeMgr;
static ClaudeAiClient  claudeAiClient;
static ButtonHandler   buttonHandler;
#if HAS_TOUCH
static TouchHandler    touchHandler;
#endif

// Screens
static ClaudeAiScreen  claudeAiScreen(state);
static StatusScreen    statusScreen(state);
static SetupScreen     setupScreen;
static LcarsBootScreen bootScreen;

static LcarsScreen* screens[] = { &claudeAiScreen, &statusScreen };
static const uint8_t SCREEN_COUNT = 2;
static uint8_t currentIdx = 0;

static DeviceMode deviceMode = MODE_SETUP;

// ============================================================
// Forward declarations
// ============================================================
void enterSetupMode();
void connectAndFetch();
void fetchData();
void handleNavigation();
void handleAutoRefresh();
void handleBacklight();
void updateWiFiState();

// ============================================================
// Setup
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ClaudeGauge v2 ===");

    memset(&state, 0, sizeof(AppState));
    state.uptime_start = millis();
    state.last_activity = millis();

    // Initialize engine (replaces DisplayManager)
    engine.begin(tft);
    engine.setTheme(LCARS_THEME_TNG);

    // Backlight
    #if defined(PIN_BL)
        pinMode(PIN_BL, OUTPUT);
        digitalWrite(PIN_BL, HIGH);
        engine.setBLPin(PIN_BL);
    #endif

    // Power pin (T-Display-S3)
    #if HAS_POWER_PIN
        pinMode(PIN_POWER_ON, OUTPUT);
        digitalWrite(PIN_POWER_ON, HIGH);
    #endif

    // Input
    buttonHandler.init();
    #if HAS_TOUCH
    touchHandler.init();
    #endif

    // Settings
    settingsMgr.begin();

    // Apply screen flip if configured (engine defaults to rotation 3)
    if (settingsMgr.getFlipScreen()) {
        tft.setRotation(1);  // 180° flip
        Serial.println("[CG] Screen flipped 180°");
    }

    // Set initial screen info
    claudeAiScreen.setScreenInfo(0, SCREEN_COUNT);

    // Check if configured
    if (!settingsMgr.hasWiFi() || !settingsMgr.hasSessionKey()) {
        enterSetupMode();
    } else {
        deviceMode = MODE_CONNECTING;
        bootScreen.setNextScreen(&claudeAiScreen);
        engine.setScreen(&bootScreen);
        connectAndFetch();
    }
}

// ============================================================
// Loop
// ============================================================
void loop() {
    engine.update();
    webServer.handleClient();

    if (webServer.shouldReboot()) {
        delay(1000);
        ESP.restart();
    }

    buttonHandler.update();
    #if HAS_TOUCH
    touchHandler.update();
    #endif

    if (deviceMode == MODE_SETUP) {
        handleBacklight();
        return;
    }

    // Boot sequence complete — switch to dashboard
    if (engine.currentScreen() == &bootScreen && bootScreen.isComplete()) {
        engine.setScreen(&claudeAiScreen);
        deviceMode = MODE_DASHBOARD;
        Serial.println("[CG] Boot complete, dashboard active");
    }

    handleNavigation();
    handleAutoRefresh();
    handleBacklight();
}

// ============================================================
// Mode transitions
// ============================================================
void enterSetupMode() {
    deviceMode = MODE_SETUP;

    // Start AP + web server (AP name generated internally)
    webServer.startAPMode();
    webServer.setEngine(&engine);
    webServer.begin(&settingsMgr);

    String apName = webServer.getAPName();
    setupScreen.setApInfo(apName.c_str(), "192.168.4.1");
    engine.setScreen(&setupScreen);

    Serial.printf("[CG] Setup mode: %s\n", apName.c_str());
}

void connectAndFetch() {
    Serial.println("[CG] Connecting to WiFi...");

    String ssid = settingsMgr.getWiFiSSID();
    String pass = settingsMgr.getWiFiPassword();

    wifiMgr.connect(ssid, pass);

    if (!wifiMgr.isConnected()) {
        Serial.println("[CG] WiFi failed, entering setup");
        enterSetupMode();
        return;
    }

    updateWiFiState();
    Serial.printf("[CG] WiFi connected: %s\n", state.wifi_ip);

    // Start web server in station mode too (for reconfiguration)
    webServer.setEngine(&engine);
    webServer.begin(&settingsMgr);

    // Sync time
    timeMgr.syncNTP();

    // Init Claude.ai client
    String sessionKey = settingsMgr.getSessionKey();
    String proxyUrl = settingsMgr.getProxyUrl();
    if (proxyUrl.isEmpty()) proxyUrl = CLAUDEAI_DEFAULT_PROXY_URL;
    claudeAiClient.init(sessionKey, proxyUrl);

    // Fetch initial data
    fetchData();
}

// ============================================================
// Data fetching
// ============================================================
void fetchData() {
    state.is_fetching = true;
    strncpy(state.fetch_status, "CLAUDE.AI...", sizeof(state.fetch_status));

    // Fetch org UUID if not cached
    if (state.claude_ai.org_uuid[0] == '\0') {
        claudeAiClient.fetchOrganization(state.claude_ai.org_uuid,
                                          sizeof(state.claude_ai.org_uuid));
    }

    // Fetch usage
    ClaudeAiUsage newUsage;
    memset(&newUsage, 0, sizeof(newUsage));
    strncpy(newUsage.org_uuid, state.claude_ai.org_uuid, sizeof(newUsage.org_uuid));

    if (claudeAiClient.fetchUsage(newUsage)) {
        state.claude_ai = newUsage;
        Serial.printf("[CG] Claude.ai: 5h=%.0f%% 7d=%.0f%%\n",
                      state.claude_ai.five_hour.utilization,
                      state.claude_ai.seven_day.utilization);
    } else {
        strncpy(state.claude_ai.error, claudeAiClient.getLastError(), ERROR_MSG_LEN);
        Serial.printf("[CG] Claude.ai error: %s\n", state.claude_ai.error);
    }

    state.is_fetching = false;
    state.last_refresh = millis();
    state.next_refresh = state.last_refresh + REFRESH_INTERVAL_MS;
}

// ============================================================
// Navigation
// ============================================================
void handleNavigation() {
    if (engine.isTransitioning()) return;

    bool nav = false;

    if (buttonHandler.isNextPressed()) {
        currentIdx = (currentIdx + 1) % SCREEN_COUNT;
        nav = true;
    }
    #if HAS_TWO_BUTTONS
    if (buttonHandler.isPrevPressed()) {
        currentIdx = (currentIdx + SCREEN_COUNT - 1) % SCREEN_COUNT;
        nav = true;
    }
    #endif

    if (nav) {
        engine.setScreen(screens[currentIdx]);
        claudeAiScreen.setScreenInfo(currentIdx, SCREEN_COUNT);
        state.last_activity = millis();
        engine.setBacklight(LCARS_BACKLIGHT_FULL);
    }

    if (buttonHandler.isRefreshPressed()) {
        fetchData();
        state.last_activity = millis();
        engine.setBacklight(LCARS_BACKLIGHT_FULL);
    }
}

// ============================================================
// Auto-refresh
// ============================================================
void handleAutoRefresh() {
    if (state.next_refresh > 0 && millis() >= state.next_refresh) {
        updateWiFiState();

        if (state.wifi_connected) {
            fetchData();
        } else {
            // Try reconnecting
            wifiMgr.reconnectIfNeeded();
            updateWiFiState();
            if (state.wifi_connected) {
                fetchData();
            } else {
                // Schedule next attempt
                state.next_refresh = millis() + REFRESH_INTERVAL_MS;
            }
        }
    }
}

// ============================================================
// Backlight dimming
// ============================================================
void handleBacklight() {
    uint32_t idle = millis() - state.last_activity;
    if (idle > LCARS_DIM_TIMEOUT_MS) {
        engine.setBacklight(LCARS_BACKLIGHT_DIM);
    }

    // Any button/touch wakes up
    #if HAS_TOUCH
    if (touchHandler.isTouched()) {
        state.last_activity = millis();
        engine.setBacklight(LCARS_BACKLIGHT_FULL);
    }
    #endif
}

// ============================================================
// WiFi state update
// ============================================================
void updateWiFiState() {
    state.wifi_connected = (WiFi.status() == WL_CONNECTED);
    if (state.wifi_connected) {
        state.wifi_rssi = WiFi.RSSI();
        IPAddress ip = WiFi.localIP();
        snprintf(state.wifi_ip, IP_LEN, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    }
}
