#include "web_server.h"
#include "config.h"
#include <WiFi.h>
#include <lcars.h>

// ============================================================
// HTML/CSS for the configuration portal
// Dark theme matching the device dashboard aesthetic
// ============================================================
static const char HTML_HEADER[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta name="claudegauge" content="1">
<title>ClaudeGauge</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
background:#0d1117;color:#e6edf3;min-height:100vh;display:flex;
justify-content:center;align-items:center;padding:20px}
.container{max-width:620px;width:100%;background:#161b22;
border-radius:12px;padding:32px;border:1px solid #30363d}
h1{color:#ff9944;font-size:22px;text-align:center;margin-bottom:2px;
display:flex;align-items:center;justify-content:center;gap:8px}
h1 svg{width:26px;height:26px;fill:#ff9944;flex-shrink:0}
.subtitle{color:#8b949e;text-align:center;font-size:13px;margin-bottom:24px}
/* Progress bar */
.progress{display:flex;align-items:center;justify-content:center;
margin-bottom:28px;gap:0}
.step-dot{width:32px;height:32px;border-radius:50%;display:flex;
align-items:center;justify-content:center;font-size:13px;font-weight:700;
border:2px solid #30363d;background:#0d1117;color:#484f58;flex-shrink:0;
transition:all .3s}
.step-dot.done{background:#00ff88;border-color:#00ff88;color:#0d1117}
.step-dot.current{border-color:#ff9944;color:#ff9944;
box-shadow:0 0 12px #ff994440}
.step-line{height:2px;width:40px;background:#30363d;flex-shrink:0}
.step-line.done{background:#00ff88}
.step-labels{display:flex;justify-content:space-between;
margin-top:6px;margin-bottom:0;padding:0 10px}
.step-label{font-size:11px;color:#484f58;text-align:center;width:70px}
.step-label.done{color:#00ff88}
.step-label.current{color:#ff9944}
/* Step cards */
.card{background:#0d1117;border:1px solid #21262d;border-radius:8px;
margin-bottom:16px;overflow:hidden}
.card.active{border-color:#ff9944}
.card-head{padding:14px 16px;display:flex;align-items:center;gap:10px;
cursor:pointer;user-select:none}
.card-head .num{width:24px;height:24px;border-radius:50%;display:flex;
align-items:center;justify-content:center;font-size:12px;font-weight:700;
background:#21262d;color:#8b949e;flex-shrink:0}
.card.done .card-head .num{background:#00ff88;color:#0d1117}
.card.active .card-head .num{background:#ff9944;color:#0d1117}
.card-head .title{font-size:14px;font-weight:600;color:#e6edf3;flex:1}
.card-head .badge{font-size:11px;padding:2px 8px;border-radius:10px;
font-weight:600}
.badge-ok{background:#00ff8820;color:#00ff88}
.badge-pending{background:#ff994420;color:#ff9944}
.badge-optional{background:#8b949e20;color:#8b949e}
.card-body{padding:0 16px 16px;display:none}
.card.open .card-body{display:block}
/* Form elements */
label{display:block;color:#8b949e;font-size:13px;margin-bottom:6px}
input[type="text"],input[type="password"]{width:100%;padding:10px 12px;
background:#161b22;border:1px solid #30363d;border-radius:6px;
color:#e6edf3;font-size:14px;margin-bottom:12px;outline:none;
transition:border-color .2s}
input:focus{border-color:#ff9944}
input::placeholder{color:#484f58}
.btn{width:100%;padding:11px;background:#ff9944;color:#0d1117;
border:none;border-radius:6px;font-size:15px;font-weight:600;
cursor:pointer;transition:background .2s}
.btn:hover{background:#ffaa66}
.btn-sm{padding:8px;font-size:13px}
.btn-danger{background:transparent;border:1px solid #f85149;
color:#f85149;margin-top:16px;font-size:13px;padding:8px}
.btn-danger:hover{background:#f8514920}
.hint{color:#484f58;font-size:12px;margin-top:2px;margin-bottom:12px}
.toggle-btn{background:none;border:none;color:#8b949e;
cursor:pointer;font-size:12px;padding:2px 6px;position:absolute;
right:12px;top:50%;transform:translateY(-50%)}
.pw-wrap{position:relative}
.pw-wrap input{padding-right:50px}
/* Extension callout */
.ext-card{background:#ff994410;border:1px solid #ff994460;
border-radius:8px;padding:16px;margin-bottom:16px}
.ext-card h3{color:#ff9944;font-size:14px;margin-bottom:4px;
display:flex;align-items:center;gap:6px}
.ext-card p{color:#8b949e;font-size:12px;line-height:1.5;margin-bottom:12px}
.ext-card .steps{color:#8b949e;font-size:12px;line-height:1.8;
margin-bottom:12px;padding-left:4px}
.ext-card .steps b{color:#e6edf3}
.btn-ext{background:#ff9944;color:#0d1117;text-decoration:none;
display:inline-block;padding:8px 20px;border-radius:6px;font-size:13px;
font-weight:600;transition:background .2s}
.btn-ext:hover{background:#ffaa66}
/* Details toggle */
details{margin-bottom:12px}
summary{color:#8b949e;font-size:13px;cursor:pointer;padding:8px 0;
list-style:none;display:flex;align-items:center;gap:6px}
summary::before{content:'\25B8';font-size:12px;transition:transform .2s}
details[open] summary::before{transform:rotate(90deg)}
summary:hover{color:#e6edf3}
/* Advanced section */
.advanced{border-top:1px solid #21262d;margin-top:8px;padding-top:16px}
/* Toggle switch */
.switch-row{display:flex;align-items:center;justify-content:space-between;
margin-bottom:12px}
.switch-row .lbl{color:#e6edf3;font-size:13px;font-weight:600}
.switch-row .sub{color:#484f58;font-size:11px;margin-top:2px}
.sw{position:relative;width:42px;height:24px;flex-shrink:0}
.sw input{opacity:0;width:0;height:0}
.sw .slider{position:absolute;inset:0;background:#30363d;border-radius:12px;
cursor:pointer;transition:.3s}
.sw .slider::before{content:'';position:absolute;width:18px;height:18px;
left:3px;bottom:3px;background:#8b949e;border-radius:50%;transition:.3s}
.sw input:checked+.slider{background:#ff994480}
.sw input:checked+.slider::before{transform:translateX(18px);background:#ff9944}
.footer{text-align:center;color:#484f58;font-size:11px;margin-top:24px}
</style>
</head>
<body>
<div class="container">
)rawliteral";

static const char HTML_FOOTER[] PROGMEM = R"rawliteral(
<div class="footer">ClaudeGauge v1.0</div>
</div>
<script>
function togglePw(id){
  var e=document.getElementById(id);
  e.type=e.type==='password'?'text':'password';
}
function toggleCard(id){
  var c=document.getElementById(id);
  c.classList.toggle('open');
}
async function captureScreen(){
  var btn=document.getElementById('btnCapture');
  var wrap=document.getElementById('screenWrap');
  var errDiv=document.getElementById('screenErr');
  var timeDiv=document.getElementById('screenTime');
  btn.textContent='Capturing...';btn.disabled=true;
  errDiv.style.display='none';
  try{
    var resp=await fetch('/screenshot');
    if(!resp.ok)throw new Error('HTTP '+resp.status);
    var buf=await resp.arrayBuffer();
    var dv=new DataView(buf);
    var w=dv.getUint16(0,true),h=dv.getUint16(2,true);
    var cv=document.getElementById('screenCanvas');
    var s=3;
    cv.width=w*s;cv.height=h*s;
    var ctx=cv.getContext('2d');
    var tmp=document.createElement('canvas');
    tmp.width=w;tmp.height=h;
    var tc=tmp.getContext('2d');
    var img=tc.createImageData(w,h);
    for(var i=0;i<w*h;i++){
      var p=dv.getUint16(4+i*2,false);
      img.data[i*4]=((p>>11)&0x1F)<<3;
      img.data[i*4+1]=((p>>5)&0x3F)<<2;
      img.data[i*4+2]=(p&0x1F)<<3;
      img.data[i*4+3]=255;
    }
    tc.putImageData(img,0,0);
    ctx.imageSmoothingEnabled=false;
    ctx.drawImage(tmp,0,0,w*s,h*s);
    wrap.style.display='block';
    document.getElementById('btnDownload').style.display='inline-block';
    timeDiv.textContent='Captured at '+new Date().toLocaleTimeString();
  }catch(e){
    errDiv.textContent='Failed: '+e.message;
    errDiv.style.display='block';
  }
  btn.textContent='Capture Screenshot';btn.disabled=false;
}
function downloadScreen(){
  var cv=document.getElementById('screenCanvas');
  if(!cv.width)return;
  var a=document.createElement('a');
  a.download='claudegauge_'+Date.now()+'.png';
  a.href=cv.toDataURL('image/png');
  a.click();
}
</script>
</body>
</html>
)rawliteral";

// ============================================================
// Web Server Implementation
// ============================================================

void ConfigWebServer::startAPMode() {
    // Create AP name with last 4 chars of MAC
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char suffix[5];
    snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
    _apName = String("ClaudeGauge-") + suffix;

    WiFi.mode(WIFI_AP);
    WiFi.softAP(_apName.c_str());

    // Start DNS server to redirect all domains to our IP (captive portal)
    _dnsServer.start(53, "*", WiFi.softAPIP());
    _apMode = true;

    Serial.printf("AP Mode started: %s\n", _apName.c_str());
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
}

void ConfigWebServer::begin(SettingsManager* settings) {
    _settings = settings;

    _server.on("/", HTTP_GET,  [this]() { handleRoot(); });
    _server.on("/save-wifi", HTTP_POST, [this]() { handleSaveWiFi(); });
    _server.on("/save-apikey", HTTP_POST, [this]() { handleSaveApiKey(); });
    _server.on("/save-session", HTTP_POST, [this]() { handleSaveSessionKey(); });
    _server.on("/save-display", HTTP_POST, [this]() { handleSaveDisplay(); });
    _server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/screenshot", HTTP_GET, [this]() { handleScreenshot(); });
    _server.on("/reset", HTTP_POST, [this]() { handleReset(); });
    _server.onNotFound([this]() { handleNotFound(); });

    _server.begin(80);
    _running = true;
    Serial.println("Web server started on port 80");
}

void ConfigWebServer::handleClient() {
    if (_running) {
        if (_apMode) {
            _dnsServer.processNextRequest();
        }
        _server.handleClient();
    }
}

void ConfigWebServer::stop() {
    if (_running) {
        _server.stop();
        _running = false;
    }
}

void ConfigWebServer::handleRoot() {
    _server.send(200, "text/html", buildPage());
}

void ConfigWebServer::handleSaveWiFi() {
    if (!_settings) {
        _server.send(500, "text/plain", "Internal error");
        return;
    }

    String ssid = _server.arg("ssid");
    String pass = _server.arg("password");

    if (ssid.length() > 0) {
        _settings->setWiFi(ssid, pass);
        Serial.printf("WiFi saved: %s\n", ssid.c_str());
        _server.send(200, "text/html", buildSuccessPage());
        _shouldReboot = true;
    } else {
        _server.send(400, "text/plain", "WiFi SSID is required");
    }
}

void ConfigWebServer::handleSaveApiKey() {
    if (!_settings) {
        _server.send(500, "text/plain", "Internal error");
        return;
    }

    String apiKey = _server.arg("apikey");

    if (apiKey.length() > 0) {
        _settings->setApiKey(apiKey);
        Serial.println("API key saved");
        _server.send(200, "text/html", buildSuccessPage());
        _shouldReboot = true;
    } else {
        _server.send(400, "text/plain", "API key is required");
    }
}

void ConfigWebServer::handleSaveSessionKey() {
    if (!_settings) {
        _server.send(500, "text/plain", "Internal error");
        return;
    }

    String sessionKey = _server.arg("sessionkey");
    String proxyUrl   = _server.arg("proxyurl");

    if (sessionKey.length() == 0) {
        _server.send(400, "text/plain", "Session key is required");
        return;
    }

    // Use default Worker URL if proxy URL not provided
    if (proxyUrl.length() == 0) {
        proxyUrl = CLAUDEAI_DEFAULT_PROXY_URL;
    }

    _settings->setSessionKey(sessionKey);
    _settings->setProxyUrl(proxyUrl);
    Serial.printf("Session key + proxy saved (proxy: %s)\n", proxyUrl.c_str());
    _server.send(200, "text/html", buildSuccessPage());
    _shouldReboot = true;
}

void ConfigWebServer::handleStatus() {
    String json = "{";
    json += "\"configured\":" + String(_settings->isConfigured() ? "true" : "false") + ",";
    json += "\"wifi_ssid\":\"" + _settings->getWiFiSSID() + "\",";
    json += "\"has_api_key\":" + String(_settings->getApiKey().length() > 0 ? "true" : "false") + ",";
    json += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += "}";
    _server.send(200, "application/json", json);
}

void ConfigWebServer::handleReset() {
    if (_settings) {
        _settings->clear();
        Serial.println("Settings cleared!");
    }
    _server.send(200, "text/html", buildSuccessPage());
    _shouldReboot = true;
}

void ConfigWebServer::handleSaveDisplay() {
    if (!_settings) {
        _server.send(500, "text/plain", "Internal error");
        return;
    }

    bool flip = _server.arg("flip") == "1";
    _settings->setFlipScreen(flip);
    Serial.printf("Display flip %s\n", flip ? "enabled" : "disabled");
    _server.send(200, "text/html", buildSuccessPage());
    _shouldReboot = true;
}

void ConfigWebServer::handleScreenshot() {
    if (!_engine) {
        _server.send(503, "text/plain", "Engine not available");
        return;
    }

    TFT_eSprite& spr = _engine->sprite();
    uint16_t w = spr.width();
    uint16_t h = spr.height();
    uint16_t* buf = (uint16_t*)spr.getPointer();

    if (!buf) {
        _server.send(503, "text/plain", "Sprite not ready");
        return;
    }

    uint32_t pixelBytes = (uint32_t)w * h * 2;
    uint32_t totalSize = 4 + pixelBytes;

    _server.setContentLength(totalSize);
    _server.sendHeader("Cache-Control", "no-cache");
    _server.send(200, "application/octet-stream", "");

    WiFiClient client = _server.client();
    // Send dimensions as little-endian uint16
    client.write((uint8_t*)&w, 2);
    client.write((uint8_t*)&h, 2);
    // Send pixel data in chunks to avoid WiFi buffer issues
    const uint32_t chunkSize = 1024;
    uint8_t* ptr = (uint8_t*)buf;
    uint32_t remaining = pixelBytes;
    while (remaining > 0) {
        uint32_t toSend = (remaining > chunkSize) ? chunkSize : remaining;
        client.write(ptr, toSend);
        ptr += toSend;
        remaining -= toSend;
    }
}

void ConfigWebServer::handleNotFound() {
    _server.sendHeader("Location", "/");
    _server.send(302, "text/plain", "");
}

String ConfigWebServer::buildPage() {
    String page;
    page.reserve(8192);

    // Gather state
    String ssid = _settings ? _settings->getWiFiSSID() : "";
    bool wifiOk = WiFi.status() == WL_CONNECTED;
    bool hasWifi = ssid.length() > 0;
    bool hasKey  = _settings && _settings->getApiKey().length() > 0;
    bool hasSess = _settings && _settings->hasSessionKey();

    // Header
    page += FPSTR(HTML_HEADER);

    // Title
    page += F("<h1><svg viewBox='0 0 24 24'><path d='M7 14a2 2 0 1 0 0-4 2 2 0 0 0 0 4Zm0 3a5 5 0 0 1-4.9-4H1v-2h1.1A5 5 0 0 1 12 9.58l7.3-7.3 2.12 2.13-2.12 2.12 1.41 1.41-2.12 2.13-1.41-1.42-1.42 1.42-1.41-1.42L12 11.42A5 5 0 0 1 7 17Z'/></svg> ClaudeGauge</h1>");
    page += F("<p class='subtitle'>Configuration Portal</p>");

    // ── Progress bar ──
    page += F("<div class='progress'>");
    // Dot 1: WiFi
    page += hasWifi ? F("<div class='step-dot done'>&#10003;</div>")
                    : F("<div class='step-dot current'>1</div>");
    // Line 1-2
    page += hasWifi ? F("<div class='step-line done'></div>")
                    : F("<div class='step-line'></div>");
    // Dot 2: API Key
    page += hasKey  ? F("<div class='step-dot done'>&#10003;</div>")
                    : (hasWifi ? F("<div class='step-dot current'>2</div>")
                               : F("<div class='step-dot'>2</div>"));
    // Line 2-3
    page += hasKey  ? F("<div class='step-line done'></div>")
                    : F("<div class='step-line'></div>");
    // Dot 3: Session
    page += hasSess ? F("<div class='step-dot done'>&#10003;</div>")
                    : (hasKey ? F("<div class='step-dot current'>3</div>")
                              : F("<div class='step-dot'>3</div>"));
    page += F("</div>");

    // Step labels
    page += F("<div class='step-labels'>");
    page += hasWifi ? F("<span class='step-label done'>WiFi</span>")
                    : F("<span class='step-label current'>WiFi</span>");
    page += hasKey  ? F("<span class='step-label done'>API Key</span>")
                    : (hasWifi ? F("<span class='step-label current'>API Key</span>")
                               : F("<span class='step-label'>API Key</span>"));
    page += hasSess ? F("<span class='step-label done'>Session</span>")
                    : (hasKey ? F("<span class='step-label current'>Session</span>")
                              : F("<span class='step-label'>Session</span>"));
    page += F("</div>");

    // ── Step 1: WiFi ──
    bool wifiOpen = !hasWifi;
    page += "<div id='c1' class='card" + String(hasWifi ? " done" : " active") +
            String(wifiOpen ? " open" : "") + "'>";
    page += F("<div class='card-head' onclick='toggleCard(\"c1\")'>");
    page += F("<div class='num'>1</div>");
    page += F("<div class='title'>WiFi Network</div>");
    if (hasWifi) {
        page += F("<span class='badge badge-ok'>");
        page += wifiOk ? "Connected" : "Saved";
        page += F("</span>");
    } else {
        page += F("<span class='badge badge-pending'>Required</span>");
    }
    page += F("</div>");
    page += F("<div class='card-body'>");
    if (hasWifi && wifiOk) {
        page += "<div style='color:#00ff88;font-size:13px;margin-bottom:12px'>"
                "Connected to <b>" + ssid + "</b> &mdash; " +
                WiFi.localIP().toString() + "</div>";
    }
    page += F("<form method='POST' action='/save-wifi'>");
    page += F("<label for='ssid'>Network Name (SSID)</label>");
    page += "<input type='text' id='ssid' name='ssid' placeholder='Enter WiFi name' value='" + ssid + "'>";
    page += F("<label for='password'>Password</label>");
    page += F("<div class='pw-wrap'>");
    page += F("<input type='password' id='password' name='password' placeholder='Enter WiFi password'>");
    page += F("<button type='button' class='toggle-btn' onclick='togglePw(\"password\")'>show</button>");
    page += F("</div>");
    page += F("<button type='submit' class='btn btn-sm'>Save WiFi &amp; Restart</button>");
    page += F("</form>");
    page += F("</div></div>");

    // ── Step 2: API Key ──
    bool keyOpen = hasWifi && !hasKey;
    page += "<div id='c2' class='card" + String(hasKey ? " done" : (hasWifi ? " active" : "")) +
            String(keyOpen ? " open" : "") + "'>";
    page += F("<div class='card-head' onclick='toggleCard(\"c2\")'>");
    page += F("<div class='num'>2</div>");
    page += F("<div class='title'>Anthropic API Key</div>");
    if (hasKey) {
        page += F("<span class='badge badge-ok'>Configured</span>");
    } else {
        page += F("<span class='badge badge-pending'>Required</span>");
    }
    page += F("</div>");
    page += F("<div class='card-body'>");
    page += F("<form method='POST' action='/save-apikey'>");
    page += F("<label for='apikey'>API Key</label>");
    page += F("<div class='pw-wrap'>");
    page += F("<input type='password' id='apikey' name='apikey' placeholder='sk-ant-admin01-...'>");
    page += F("<button type='button' class='toggle-btn' onclick='togglePw(\"apikey\")'>show</button>");
    page += F("</div>");
    page += F("<div class='hint'>Admin key from console.anthropic.com for usage and cost data.</div>");
    page += F("<button type='submit' class='btn btn-sm'>Save API Key &amp; Restart</button>");
    page += F("</form>");
    page += F("</div></div>");

    // ── Step 3: Claude.ai Session ──
    bool sessOpen = hasKey && !hasSess;
    page += "<div id='c3' class='card" + String(hasSess ? " done" : (hasKey ? " active" : "")) +
            String(sessOpen ? " open" : "") + "'>";
    page += F("<div class='card-head' onclick='toggleCard(\"c3\")'>");
    page += F("<div class='num'>3</div>");
    page += F("<div class='title'>Claude.ai Session</div>");
    if (hasSess) {
        page += F("<span class='badge badge-ok'>Configured</span>");
    } else {
        page += F("<span class='badge badge-optional'>Optional</span>");
    }
    page += F("</div>");
    page += F("<div class='card-body'>");

    page += F("<div class='hint' style='margin-bottom:14px'>"
              "Tracks your Claude.ai subscription rate limits (5h and 7-day usage, model limits).</div>");

    // Extension callout
    page += F("<div class='ext-card'>"
              "<h3>&#128268; Recommended: Browser Extension</h3>"
              "<p>Install our Chrome extension for one-click session key setup. "
              "No developer tools needed.</p>"
              "<div class='steps'>"
              "<b>1.</b> Install the extension from the Chrome Web Store<br>"
              "<b>2.</b> Log into <a href='https://claude.ai' target='_blank' "
              "style='color:#ff9944'>claude.ai</a> in your browser<br>"
              "<b>3.</b> Come back to this page<br>"
              "<b>4.</b> Click the <b style='color:#ff9944'>Auto-fill</b> button that appears below"
              "</div>"
              "<a href='#EXTENSION_URL#' target='_blank' class='btn-ext'>"
              "Install Extension</a>"
              "</div>");

    // Manual instructions (collapsed)
    page += F("<details>"
              "<summary>Manual setup (advanced)</summary>"
              "<div style='padding:10px 0;font-size:12px;color:#8b949e;line-height:1.7'>"
              "<b style='color:#e6edf3'>1.</b> Open "
              "<a href='https://claude.ai' style='color:#ff9944'>claude.ai</a> and log in<br>"
              "<b style='color:#e6edf3'>2.</b> Press <b style='color:#e6edf3'>F12</b> to open DevTools<br>"
              "<b style='color:#e6edf3'>3.</b> Go to <b style='color:#e6edf3'>Application</b> tab "
              "&rarr; <b style='color:#e6edf3'>Cookies</b> &rarr; "
              "<b style='color:#e6edf3'>https://claude.ai</b><br>"
              "<b style='color:#e6edf3'>4.</b> Find <b style='color:#00ff88'>sessionKey</b> "
              "and copy its <b style='color:#e6edf3'>Value</b><br>"
              "<b style='color:#e6edf3'>5.</b> Paste it below"
              "</div></details>");

    // Session key input + save
    page += F("<form method='POST' action='/save-session'>");
    page += F("<label for='sessionkey'>Session Key</label>");
    page += F("<div class='pw-wrap'>");
    page += F("<input type='password' id='sessionkey' name='sessionkey' placeholder='sk-ant-sid01-...'>");
    page += F("<button type='button' class='toggle-btn' onclick='togglePw(\"sessionkey\")'>show</button>");
    page += F("</div>");

    // Proxy URL hidden in the form as a hidden field with default value
    String proxyUrl = _settings ? _settings->getProxyUrl() : "";
    if (proxyUrl.length() == 0) proxyUrl = CLAUDEAI_DEFAULT_PROXY_URL;
    page += "<input type='hidden' name='proxyurl' value='" + proxyUrl + "'>";

    page += F("<button type='submit' class='btn btn-sm'>Save Session &amp; Restart</button>");
    page += F("</form>");
    page += F("</div></div>");

    // ── Advanced Settings ──
    page += F("<details style='margin-top:8px'>"
              "<summary>Advanced Settings</summary>"
              "<div class='advanced'>");

    // Flip screen toggle
    bool isFlipped = _settings && _settings->getFlipScreen();
    page += F("<form method='POST' action='/save-display'>"
              "<div class='switch-row'>"
              "<div><div class='lbl'>Flip Screen 180&deg;</div>"
              "<div class='sub'>For alternate mounting orientation</div></div>"
              "<label class='sw'>");
    page += String("<input type='checkbox' name='flip' value='1' onchange='this.form.submit()'") +
            (isFlipped ? " checked" : "") + ">";
    page += F("<span class='slider'></span></label></div>"
              "<input type='hidden' name='flip' value='0'>"
              "</form>");

    page += F("<form method='POST' action='/save-session'>");
    page += F("<label for='proxyurl'>Proxy URL</label>");
    page += "<input type='text' id='proxyurl' name='proxyurl' value='" + proxyUrl + "'>";
    page += F("<div class='hint'>Cloud proxy is pre-configured. For a local proxy: "
              "<code style='color:#e6edf3'>http://&lt;IP&gt;:8081</code></div>");
    page += F("<input type='hidden' name='sessionkey' id='sk2' value=''>");
    page += F("<button type='submit' class='btn btn-sm' "
              "onclick='document.getElementById(\"sk2\").value="
              "document.getElementById(\"sessionkey\").value;"
              "return true;'>Save Proxy URL &amp; Restart</button>");
    page += F("</form>");

    page += F("</div></details>");

    // ── Display Preview (Screenshot) ──
    page += F("<div id='cScreen' class='card open' style='margin-top:16px'>"
              "<div class='card-head' onclick='toggleCard(\"cScreen\")'>"
              "<div class='num' style='background:#ff9944;color:#0d1117'>&#128247;</div>"
              "<div class='title'>Display Preview</div>"
              "</div>"
              "<div class='card-body'>"
              "<div style='text-align:center'>"
              "<button class='btn btn-sm' onclick='captureScreen()' id='btnCapture'"
              " style='width:auto;padding:8px 24px;margin-bottom:12px'>Capture Screenshot</button>"
              " <button class='btn btn-sm' onclick='downloadScreen()' id='btnDownload'"
              " style='width:auto;padding:8px 24px;margin-bottom:12px;display:none;"
              "background:#30363d;color:#e6edf3'>Download PNG</button>"
              "<div id='screenWrap' style='display:none;margin-top:8px;"
              "overflow-x:auto'>"
              "<canvas id='screenCanvas'"
              " style='border:1px solid #30363d;border-radius:4px;"
              "background:#000;image-rendering:pixelated;max-width:100%;height:auto'></canvas>"
              "<div style='color:#484f58;font-size:11px;margin-top:6px'>"
              "<span id='screenTime'></span></div>"
              "</div>"
              "<div id='screenErr' style='color:#f85149;font-size:13px;display:none'></div>"
              "</div></div></div>");

    // Reset button
    page += F("<form method='POST' action='/reset'>");
    page += F("<button type='submit' class='btn btn-danger' ");
    page += F("onclick='return confirm(\"Clear all settings and restart?\")'>Reset All Settings</button>");
    page += F("</form>");

    // Footer
    page += FPSTR(HTML_FOOTER);

    return page;
}

String ConfigWebServer::buildSuccessPage() {
    String page;
    page.reserve(1024);

    page += FPSTR(HTML_HEADER);
    page += F("<h1>Settings Saved</h1>");
    page += F("<p class='subtitle'>Device is restarting...</p>");
    page += F("<div style='text-align:center;margin:40px 0'>");
    page += F("<div style='color:#00ff88;font-size:48px'>&#10003;</div>");
    page += F("<p style='color:#8b949e;margin-top:16px'>The device will reboot and connect with the new settings.</p>");
    page += F("<p style='color:#484f58;margin-top:12px;font-size:13px'>If WiFi settings changed, reconnect to your network to access the dashboard.</p>");
    page += F("</div>");
    page += FPSTR(HTML_FOOTER);

    return page;
}
