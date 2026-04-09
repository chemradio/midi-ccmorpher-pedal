#pragma once
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include "../footswitches/footswitchObject.h"
#include "../pedalState.h"
#include "../statePersistance.h"
#include "webUI.h"

// ── AP credentials ─────────────────────────────────────────────────────────────

static constexpr char WIFI_AP_SSID[] = "MIDI Morpher";
static constexpr char WIFI_AP_PASS[] = "midimorpher";

// ── Module state ───────────────────────────────────────────────────────────────

inline WebServer    webServer(80);
inline PedalState  *_webPedal    = nullptr;
inline bool         _wifiEnabled = true;
inline bool         _apRunning   = false;
inline bool         _wasLocked   = false;
inline unsigned long _holdStart  = 0;  // FS1+FS2 hold timer for WiFi recovery

// ── NVS helpers ────────────────────────────────────────────────────────────────

inline bool loadWifiEnabled() {
    Preferences p;
    p.begin("wifi", true);
    bool en = p.getBool("on", true);
    p.end();
    return en;
}

inline void saveWifiEnabled(bool en) {
    Preferences p;
    p.begin("wifi", false);
    p.putBool("on", en);
    p.end();
}

// ── WiFi LED ───────────────────────────────────────────────────────────────────

inline void setWifiLed(bool on) {
    digitalWrite(WIFI_LED_PIN, on ? HIGH : LOW);
}

// ── AP control ─────────────────────────────────────────────────────────────────

inline void startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    webServer.begin();
    _apRunning = true;
    setWifiLed(true);
}

inline void stopAP() {
    webServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    _apRunning = false;
    setWifiLed(false);
}

// ── JSON helpers ───────────────────────────────────────────────────────────────
// Minimal hand-rolled parser — no external library needed.

inline int jsonInt(const String &body, const char *key) {
    String k = String('"') + key + '"';
    int pos  = body.indexOf(k);
    if (pos < 0) return -9999;
    pos = body.indexOf(':', pos + k.length());
    if (pos < 0) return -9999;
    pos++;
    while (pos < (int)body.length() && body[pos] == ' ') pos++;
    bool neg = false;
    if (pos < (int)body.length() && body[pos] == '-') { neg = true; pos++; }
    String num;
    while (pos < (int)body.length() && isDigit(body[pos])) num += body[pos++];
    if (num.isEmpty()) return -9999;
    int v = num.toInt();
    return neg ? -v : v;
}

inline bool jsonBool(const String &body, const char *key, bool &out) {
    String k = String('"') + key + '"';
    int pos  = body.indexOf(k);
    if (pos < 0) return false;
    pos = body.indexOf(':', pos + k.length());
    if (pos < 0) return false;
    pos++;
    while (pos < (int)body.length() && body[pos] == ' ') pos++;
    if (body.substring(pos, pos + 4) == "true")  { out = true;  return true; }
    if (body.substring(pos, pos + 5) == "false") { out = false; return true; }
    return false;
}

// ── Mode application ───────────────────────────────────────────────────────────
// Mirrors the logic in toggleFootswitchMode but sets a specific index directly.

inline void applyModeIndex(FSButton &btn, uint8_t idx) {
    if (idx >= NUM_MODES) return;
    const ModeInfo &m = modes[idx];
    btn.modeIndex   = idx;
    btn.mode        = m.mode;
    btn.isModSwitch = m.isModSwitch;
    btn.isLatching  = m.isLatching;
    btn.isPC        = m.isPC;
    btn.isNote      = m.isNote;
    btn.isScene     = m.isScene;
    btn.modMode     = m;
    btn.isActivated = false;
    btn._setLED(false);
    if (_webPedal) {
        MidiCCModulator &mod = _webPedal->modulator;
        if (m.isInverted) mod.setRestingHigh();
        else              mod.setRestingLow();
        mod.rampShape = m.shape;
        mod.reset();
    }
}

// ── State JSON builder ─────────────────────────────────────────────────────────

inline String buildStateJson() {
    if (!_webPedal) return F("{}");
    const PedalState &p = *_webPedal;
    String j;
    j.reserve(512);
    j  = F("{\"channel\":");
    j += p.midiChannel;
    j += F(",\"wifiEnabled\":");
    j += _wifiEnabled ? F("true") : F("false");
    j += F(",\"buttons\":[");
    for (int i = 0; i < (int)p.buttons.size(); i++) {
        const FSButton &b = p.buttons[i];
        if (i) j += ',';
        j += F("{\"id\":");       j += i;
        j += F(",\"name\":\"");   j += b.name;         j += '"';
        j += F(",\"modeIndex\":"); j += b.modeIndex;
        j += F(",\"modeName\":\""); j += modes[b.modeIndex].name; j += '"';
        j += F(",\"midiNumber\":"); j += b.midiNumber;
        j += F(",\"fsChannel\":"); j += b.fsChannel;
        j += F(",\"rampUpMs\":"); j += b.rampUpMs;
        j += F(",\"rampDownMs\":"); j += b.rampDownMs;
        j += F(",\"isModSwitch\":"); j += b.isModSwitch ? F("true") : F("false");
        j += '}';
    }
    j += F("]}");
    return j;
}

// ── CORS / options helper ──────────────────────────────────────────────────────

inline void addCORS() {
    webServer.sendHeader(F("Access-Control-Allow-Origin"),  F("*"));
    webServer.sendHeader(F("Access-Control-Allow-Methods"), F("GET,POST,OPTIONS"));
    webServer.sendHeader(F("Access-Control-Allow-Headers"), F("Content-Type"));
}

inline void handleOPTIONS() {
    addCORS();
    webServer.send(200);
}

// ── Route handlers ─────────────────────────────────────────────────────────────

inline void handleRoot() {
    addCORS();
    webServer.send_P(200, "text/html", WEB_UI_HTML);
}

inline void handleGetState() {
    addCORS();
    webServer.send(200, F("application/json"), buildStateJson());
}

inline void handlePostChannel() {
    addCORS();
    if (!_webPedal) { webServer.send(500); return; }
    int ch = jsonInt(webServer.arg("plain"), "channel");
    if (ch < 0 || ch > 15) { webServer.send(400, F("application/json"), F("{\"error\":\"bad channel\"}")); return; }
    _webPedal->setMidiChannel((uint8_t)ch);
    markStateDirty();
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePostButton(int idx) {
    addCORS();
    if (!_webPedal) { webServer.send(500); return; }
    if (idx < 0 || idx >= (int)_webPedal->buttons.size()) { webServer.send(400); return; }
    FSButton &btn    = _webPedal->buttons[idx];
    const String &body = webServer.arg("plain");
    int mi = jsonInt(body, "modeIndex");
    int mn = jsonInt(body, "midiNumber");
    int ch = jsonInt(body, "fsChannel");
    int up = jsonInt(body, "rampUpMs");
    int dn = jsonInt(body, "rampDownMs");
    if (mi >= 0 && mi < NUM_MODES)             applyModeIndex(btn, (uint8_t)mi);
    if (mn >= 0 && mn <= 127)                  btn.midiNumber = (uint8_t)mn;
    if (ch == 255 || (ch >= 0 && ch <= 15))    btn.fsChannel  = (uint8_t)ch;
    if (up >= 0 && up <= 5000)                 btn.rampUpMs   = (uint32_t)up;
    if (dn >= 0 && dn <= 5000)                 btn.rampDownMs = (uint32_t)dn;
    markStateDirty();
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePostWifi() {
    addCORS();
    bool en = true;
    if (!jsonBool(webServer.arg("plain"), "enabled", en)) {
        webServer.send(400, F("application/json"), F("{\"error\":\"bad body\"}"));
        return;
    }
    _wifiEnabled = en;
    saveWifiEnabled(en);
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
    if (!en) stopAP();
}

// ── Init ───────────────────────────────────────────────────────────────────────

inline void initWebServer(PedalState &pedal) {
    _webPedal    = &pedal;
    _wifiEnabled = loadWifiEnabled();

    pinMode(WIFI_LED_PIN, OUTPUT);
    setWifiLed(false);

    webServer.on("/",              HTTP_GET,     handleRoot);
    webServer.on("/api/state",     HTTP_GET,     handleGetState);
    webServer.on("/api/channel",   HTTP_POST,    handlePostChannel);
    webServer.on("/api/channel",   HTTP_OPTIONS, handleOPTIONS);
    webServer.on("/api/wifi",      HTTP_POST,    handlePostWifi);
    webServer.on("/api/wifi",      HTTP_OPTIONS, handleOPTIONS);

    for (int i = 0; i < 6; i++) {
        char path[16];
        snprintf(path, sizeof(path), "/api/button/%d", i);
        webServer.on(path, HTTP_POST,    [i]() { handlePostButton(i); });
        webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
    }

    _wasLocked = pedal.settingsLocked;

    if (_wifiEnabled && !pedal.settingsLocked) {
        startAP();
    }
}

// ── Loop ───────────────────────────────────────────────────────────────────────

inline void handleWebServer(PedalState &pedal) {
    // Sync WiFi to lock switch transitions
    if (pedal.settingsLocked != _wasLocked) {
        _wasLocked = pedal.settingsLocked;
        if (pedal.settingsLocked && _apRunning) {
            stopAP();
        } else if (!pedal.settingsLocked && _wifiEnabled && !_apRunning) {
            startAP();
        }
    }

    // Recovery: hold FS1 + FS2 for 3 s to force WiFi back on when disabled via UI
    bool held = pedal.buttons[0].isPressed && pedal.buttons[1].isPressed;
    if (held) {
        if (_holdStart == 0) _holdStart = millis();
        else if (!_wifiEnabled && (millis() - _holdStart) >= 3000) {
            _wifiEnabled = true;
            saveWifiEnabled(true);
            if (!pedal.settingsLocked) startAP();
            _holdStart = 0;
        }
    } else {
        _holdStart = 0;
    }

    if (_apRunning) webServer.handleClient();
}
