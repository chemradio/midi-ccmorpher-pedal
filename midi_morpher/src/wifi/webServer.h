#pragma once
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
inline WebServer   webServer(80);
inline PedalState *_webPedal  = nullptr;
inline bool        _apRunning = false;
inline bool        _wasLocked = false;

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
inline int jsonInt(const String &body, const char *key) {
    String k = String('"') + key + '"';
    int pos  = body.indexOf(k);
    if(pos < 0) return -9999;
    pos = body.indexOf(':', pos + k.length());
    if(pos < 0) return -9999;
    pos++;
    while(pos < (int)body.length() && body[pos] == ' ') pos++;
    bool neg = false;
    if(pos < (int)body.length() && body[pos] == '-') { neg = true; pos++; }
    String num;
    while(pos < (int)body.length() && isDigit(body[pos])) num += body[pos++];
    if(num.isEmpty()) return -9999;
    int v = num.toInt();
    return neg ? -v : v;
}

inline bool jsonBool(const String &body, const char *key, bool &out) {
    String k = String('"') + key + '"';
    int pos  = body.indexOf(k);
    if(pos < 0) return false;
    pos = body.indexOf(':', pos + k.length());
    if(pos < 0) return false;
    pos++;
    while(pos < (int)body.length() && body[pos] == ' ') pos++;
    if(body.substring(pos, pos + 4) == "true")  { out = true;  return true; }
    if(body.substring(pos, pos + 5) == "false") { out = false; return true; }
    return false;
}

// ── State JSON builders ────────────────────────────────────────────────────────

inline String buildStateJson() {
    if(!_webPedal) return F("{}");
    const PedalState &p = *_webPedal;
    String j;
    j.reserve(700);
    j  = F("{\"channel\":");
    j += p.midiChannel;
    j += F(",\"activePreset\":");
    j += activePreset;
    j += F(",\"presetDirty\":");
    j += presetDirty ? F("true") : F("false");
    j += F(",\"latching\":");
    j += p.modulator.latching ? F("true") : F("false");
    j += F(",\"buttons\":[");
    for(int i = 0; i < (int)p.buttons.size(); i++) {
        const FSButton &b = p.buttons[i];
        if(i) j += ',';
        j += F("{\"id\":");        j += i;
        j += F(",\"name\":\"");    j += b.name;           j += '"';
        j += F(",\"modeIndex\":"); j += b.modeIndex;
        j += F(",\"modeName\":\""); j += modes[b.modeIndex].name; j += '"';
        j += F(",\"midiNumber\":"); j += b.midiNumber;
        j += F(",\"fsChannel\":"); j += b.fsChannel;
        j += F(",\"rampUpMs\":"); j += b.rampUpMs;
        j += F(",\"rampDownMs\":"); j += b.rampDownMs;
        j += F(",\"isModSwitch\":"); j += b.isModSwitch ? F("true") : F("false");
        j += F(",\"isLatching\":"); j += b.isLatching ? F("true") : F("false");
        j += '}';
    }
    j += F("]}");
    return j;
}

inline String buildPresetsJson() {
    String j;
    j.reserve(2800);
    j  = F("{\"activePreset\":");
    j += activePreset;
    j += F(",\"presets\":[");
    for(int p = 0; p < NUM_PRESETS; p++) {
        if(p) j += ',';
        j += F("{\"midiChannel\":");
        j += presets[p].midiChannel;
        j += F(",\"buttons\":[");
        for(int i = 0; i < 6; i++) {
            if(i) j += ',';
            const FSButtonPersisted &b = presets[p].buttons[i];
            j += F("{\"modeIndex\":"); j += b.modeIndex;
            j += F(",\"midiNumber\":"); j += b.midiNumber;
            j += F(",\"rampUpMs\":"); j += b.rampUpMs;
            j += F(",\"rampDownMs\":"); j += b.rampDownMs;
            j += F(",\"fsChannel\":"); j += b.fsChannel;
            j += '}';
        }
        j += F("]}");
    }
    j += F("]}");
    return j;
}

// ── CORS helper ────────────────────────────────────────────────────────────────
inline void addCORS() {
    webServer.sendHeader(F("Access-Control-Allow-Origin"),  F("*"));
    webServer.sendHeader(F("Access-Control-Allow-Methods"), F("GET,POST,OPTIONS"));
    webServer.sendHeader(F("Access-Control-Allow-Headers"), F("Content-Type"));
}

inline void handleOPTIONS() { addCORS(); webServer.send(200); }

// ── Route handlers ─────────────────────────────────────────────────────────────

inline void handleRoot() {
    addCORS();
    webServer.send_P(200, "text/html", WEB_UI_HTML);
}

inline void handleGetState() {
    addCORS();
    webServer.send(200, F("application/json"), buildStateJson());
}

inline void handleGetPresets() {
    addCORS();
    webServer.send(200, F("application/json"), buildPresetsJson());
}

inline void handlePostChannel() {
    addCORS();
    if(!_webPedal) { webServer.send(500); return; }
    int ch = jsonInt(webServer.arg("plain"), "channel");
    if(ch < 0 || ch > 15) { webServer.send(400, F("application/json"), F("{\"error\":\"bad channel\"}")); return; }
    _webPedal->setMidiChannel((uint8_t)ch);
    markStateDirty();
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePostButton(int idx) {
    addCORS();
    if(!_webPedal) { webServer.send(500); return; }
    if(idx < 0 || idx >= (int)_webPedal->buttons.size()) { webServer.send(400); return; }
    FSButton &btn     = _webPedal->buttons[idx];
    const String &body = webServer.arg("plain");
    int mi = jsonInt(body, "modeIndex");
    int mn = jsonInt(body, "midiNumber");
    int ch = jsonInt(body, "fsChannel");
    int up = jsonInt(body, "rampUpMs");
    int dn = jsonInt(body, "rampDownMs");
    if(mi >= 0 && mi < NUM_MODES)           applyModeIndex(btn, (uint8_t)mi, &_webPedal->modulator);
    if(mn >= 0 && mn <= 127)                btn.midiNumber = (uint8_t)mn;
    if(ch == 255 || (ch >= 0 && ch <= 15))  btn.fsChannel  = (uint8_t)ch;
    if(up >= 0 && up <= 5000)               btn.rampUpMs   = (uint32_t)up;
    if(dn >= 0 && dn <= 5000)               btn.rampDownMs = (uint32_t)dn;
    markStateDirty();
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handleButtonPress(int idx) {
    addCORS();
    if(!_webPedal || idx < 0 || idx >= 6) { webServer.send(400); return; }
    FSButton &btn = _webPedal->buttons[idx];
    btn.simulatePress(true, _webPedal->midiChannel, _webPedal->modulator);
    _webPedal->lastActiveFSIndex = idx;
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handleButtonRelease(int idx) {
    addCORS();
    if(!_webPedal || idx < 0 || idx >= 6) { webServer.send(400); return; }
    FSButton &btn = _webPedal->buttons[idx];
    btn.simulatePress(false, _webPedal->midiChannel, _webPedal->modulator);
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePostPot() {
    addCORS();
    if(!_webPedal) { webServer.send(500); return; }
    const String &body = webServer.arg("plain");
    int id    = jsonInt(body, "id");
    int value = jsonInt(body, "value");
    if(id < 0 || id > 1 || value < 0 || value > 127) { webServer.send(400); return; }
    uint8_t cc = (id == 0) ? POT1_CC : POT2_CC;
    sendMIDI(_webPedal->midiChannel, false, cc, (uint8_t)value);
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePostLatching() {
    addCORS();
    if(!_webPedal) { webServer.send(500); return; }
    bool latching = false;
    if(!jsonBool(webServer.arg("plain"), "latching", latching)) { webServer.send(400); return; }
    _webPedal->modulator.setLatch(latching);
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePresetLoad(int idx) {
    addCORS();
    if(!_webPedal || idx < 0 || idx >= NUM_PRESETS) { webServer.send(400); return; }
    applyPreset((uint8_t)idx, *_webPedal);
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePresetSave(int idx) {
    addCORS();
    if(!_webPedal || idx < 0 || idx >= NUM_PRESETS) { webServer.send(400); return; }
    if(_webPedal->settingsLocked) {
        webServer.send(403, F("application/json"), F("{\"error\":\"locked\"}"));
        return;
    }
    activePreset = (uint8_t)idx;
    saveCurrentPreset(*_webPedal);
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

// ── Init ───────────────────────────────────────────────────────────────────────
inline void initWebServer(PedalState &pedal) {
    _webPedal = &pedal;

    pinMode(WIFI_LED_PIN, OUTPUT);
    setWifiLed(false);

    webServer.on("/",              HTTP_GET,  handleRoot);
    webServer.on("/api/state",     HTTP_GET,  handleGetState);
    webServer.on("/api/presets",   HTTP_GET,  handleGetPresets);
    webServer.on("/api/presets",   HTTP_OPTIONS, handleOPTIONS);
    webServer.on("/api/channel",   HTTP_POST, handlePostChannel);
    webServer.on("/api/channel",   HTTP_OPTIONS, handleOPTIONS);
    webServer.on("/api/pot",       HTTP_POST, handlePostPot);
    webServer.on("/api/pot",       HTTP_OPTIONS, handleOPTIONS);
    webServer.on("/api/latching",  HTTP_POST, handlePostLatching);
    webServer.on("/api/latching",  HTTP_OPTIONS, handleOPTIONS);

    for(int i = 0; i < 6; i++) {
        char path[26];
        snprintf(path, sizeof(path), "/api/button/%d", i);
        webServer.on(path, HTTP_POST,    [i]() { handlePostButton(i); });
        webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
        snprintf(path, sizeof(path), "/api/button/%d/press", i);
        webServer.on(path, HTTP_POST,    [i]() { handleButtonPress(i); });
        webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
        snprintf(path, sizeof(path), "/api/button/%d/release", i);
        webServer.on(path, HTTP_POST,    [i]() { handleButtonRelease(i); });
        webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
    }

    for(int i = 0; i < NUM_PRESETS; i++) {
        char path[26];
        snprintf(path, sizeof(path), "/api/preset/load/%d", i);
        webServer.on(path, HTTP_POST,    [i]() { handlePresetLoad(i); });
        webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
        snprintf(path, sizeof(path), "/api/preset/save/%d", i);
        webServer.on(path, HTTP_POST,    [i]() { handlePresetSave(i); });
        webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
    }

    _wasLocked = pedal.settingsLocked;

    // WiFi is always on unless the pedal is locked at boot.
    if(!pedal.settingsLocked) {
        startAP();
    }
}

// ── Loop ───────────────────────────────────────────────────────────────────────
inline void handleWebServer(PedalState &pedal) {
    // Sync WiFi to lock switch: stop when locked, start when unlocked.
    if(pedal.settingsLocked != _wasLocked) {
        _wasLocked = pedal.settingsLocked;
        if(pedal.settingsLocked && _apRunning) {
            stopAP();
        } else if(!pedal.settingsLocked && !_apRunning) {
            startAP();
        }
    }

    if(_apRunning) webServer.handleClient();
}
