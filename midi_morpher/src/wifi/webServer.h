#pragma once
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include "../clock/midiClock.h"
#include "../controls/pots.h"
#include "../footswitches/footswitchObject.h"
#include "../pedalState.h"
#include "../statePersistance.h"
#include "../analogInOut/expInput.h"
#include "webUI.h"

// ── AP credentials ─────────────────────────────────────────────────────────────
static constexpr char WIFI_AP_SSID[] = "MIDI Morpher";
static constexpr char WIFI_AP_PASS[] = "midimorpher";

// ── Module state ───────────────────────────────────────────────────────────────
inline WebServer   webServer(80);
inline DNSServer   dnsServer;
inline PedalState *_webPedal  = nullptr;
inline bool        _apRunning = false;
inline bool        _wasLocked = false;

// AP gateway / captive-portal target address.
static const IPAddress AP_IP(192, 168, 4, 1);

// ── AP control ─────────────────────────────────────────────────────────────────
inline void startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    // Catch-all DNS — every hostname resolves to the pedal, so any URL the
    // phone/laptop probes for captive-portal detection hits our web server.
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", AP_IP);
    MDNS.begin("midimorpher");
    webServer.begin();
    _apRunning = true;
}

inline void stopAP() {
    MDNS.end();
    dnsServer.stop();
    webServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    _apRunning = false;
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

// Parse an unsigned 32-bit int. Returns 0xFFFFFFFF if the key is absent or
// unparseable. Needed for rampUpMs/rampDownMs which may have bit 31 set
// (CLOCK_SYNC_FLAG) and therefore overflow a plain signed int.
inline uint32_t jsonUint(const String &body, const char *key) {
    String k = String('"') + key + '"';
    int pos  = body.indexOf(k);
    if(pos < 0) return 0xFFFFFFFFUL;
    pos = body.indexOf(':', pos + k.length());
    if(pos < 0) return 0xFFFFFFFFUL;
    pos++;
    while(pos < (int)body.length() && body[pos] == ' ') pos++;
    uint32_t val = 0;
    bool gotDigits = false;
    while(pos < (int)body.length() && isDigit(body[pos])) {
        val = val * 10 + (uint32_t)(body[pos] - '0');
        pos++;
        gotDigits = true;
    }
    return gotDigits ? val : 0xFFFFFFFFUL;
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
    j.reserve(1200);
    j  = F("{\"channel\":");
    j += p.midiChannel;
    j += F(",\"activePreset\":");
    j += activePreset;
    j += F(",\"presetDirty\":");
    j += presetDirty ? F("true") : F("false");
    j += F(",\"bpm\":");
    j += (int)midiClock.bpm;
    j += F(",\"externalSync\":");
    j += midiClock.externalSync ? F("true") : F("false");
    j += F(",\"buttons\":[");
    for(int i = 0; i < (int)p.buttons.size(); i++) {
        const FSButton &b = p.buttons[i];
        if(i) j += ',';
        bool upSync = (b.rampUpMs   & CLOCK_SYNC_FLAG) != 0;
        bool dnSync = (b.rampDownMs & CLOCK_SYNC_FLAG) != 0;
        j += F("{\"id\":");              j += i;
        j += F(",\"name\":\"");          j += b.name;                   j += '"';
        j += F(",\"modeIndex\":");       j += b.modeIndex;
        j += F(",\"modeName\":\"");      j += modes[b.modeIndex].name;  j += '"';
        j += F(",\"midiNumber\":");      j += b.midiNumber;
        j += F(",\"fsChannel\":");       j += b.fsChannel;
        j += F(",\"rampUpMs\":");        j += upSync ? 0 : (uint32_t)b.rampUpMs;
        j += F(",\"rampDownMs\":");      j += dnSync ? 0 : (uint32_t)b.rampDownMs;
        j += F(",\"rampUpSync\":");      j += upSync ? F("true") : F("false");
        j += F(",\"rampUpNoteIdx\":");   j += (uint8_t)(b.rampUpMs   & 0xFF);
        j += F(",\"rampDownSync\":");    j += dnSync ? F("true") : F("false");
        j += F(",\"rampDownNoteIdx\":"); j += (uint8_t)(b.rampDownMs & 0xFF);
        j += F(",\"isModSwitch\":");     j += b.isModSwitch ? F("true") : F("false");
        j += F(",\"isLatching\":");      j += b.isLatching  ? F("true") : F("false");
        j += F(",\"isKeyboard\":");      j += b.isKeyboard  ? F("true") : F("false");
        j += F(",\"ccLow\":");           j += b.ccLow;
        j += F(",\"ccHigh\":");          j += b.ccHigh;
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

// Captive-portal catch-all. Any URL other than the registered ones — including
// OS probes like /generate_204, /hotspot-detect.html, /ncsi.txt — gets a 302
// redirect to the root, which triggers the "Sign in to network" prompt on the
// phone/laptop and takes the user straight to the UI.
inline void handleCaptivePortal() {
    webServer.sendHeader(F("Location"), F("http://192.168.4.1/"), true);
    webServer.send(302, F("text/plain"), F(""));
}

// Dismiss the captive-portal websheet without disconnecting from WiFi.
// Returning 204 satisfies Android's connectivity check; iOS shows its own
// Done button in the websheet chrome.
inline void handleDismiss() {
    webServer.send(204, F("text/plain"), F(""));
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

// Ramp value accepts either plain ms (0–5000) or CLOCK_SYNC_FLAG | noteIdx.
// Returns true if the value is a valid ramp.
inline bool _validRamp(uint32_t v) {
    if(v == 0xFFFFFFFFUL) return false;
    if(v & CLOCK_SYNC_FLAG) return (v & 0xFF) < NUM_NOTE_VALUES;
    return v <= 5000;
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
    uint32_t up = jsonUint(body, "rampUpMs");
    uint32_t dn = jsonUint(body, "rampDownMs");
    if(mi >= 0 && mi < NUM_MODES)           applyModeIndex(btn, (uint8_t)mi, &_webPedal->modulator);
    // Clamp midiNumber against mode-appropriate max. Scene modes cap at
    // sceneMaxVal (7 for Helix/QC/Fractal, 4 for Kemper); system commands cap
    // at NUM_SYS_CMDS-1; modulation modes additionally accept 255 (PB_SENTINEL)
    // meaning Pitch Bend destination; everything else 0–127.
    if(btn.isModSwitch && mn == (int)PB_SENTINEL) {
      btn.midiNumber = PB_SENTINEL;
    } else {
      int mnMax = btn.isScene  ? (int)btn.modMode.sceneMaxVal
                : btn.isSystem ? (int)(NUM_SYS_CMDS - 1)
                : 127;
      if(mn >= 0 && mn <= mnMax)            btn.midiNumber = (uint8_t)mn;
    }
    if(ch == 255 || (ch >= 0 && ch <= 15))  btn.fsChannel  = (uint8_t)ch;
    if(_validRamp(up))                      btn.rampUpMs   = up;
    if(_validRamp(dn))                      btn.rampDownMs = dn;
    int cl = jsonInt(body, "ccLow");
    int ch2 = jsonInt(body, "ccHigh");
    if(cl >= 0 && cl <= 127)   btn.ccLow  = (uint8_t)cl;
    if(ch2 >= 0 && ch2 <= 127) btn.ccHigh = (uint8_t)ch2;
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
    uint8_t cc = (id == 0) ? _webPedal->globalSettings.pot1CC
                           : _webPedal->globalSettings.pot2CC;
    if(cc != POT_CC_OFF)
        sendMIDI(_webPedal->midiChannel, false, cc, (uint8_t)value);
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

// ── Global settings ────────────────────────────────────────────────────────────

inline String buildGlobalJson() {
    if(!_webPedal) return F("{}");
    const GlobalSettings &gs = _webPedal->globalSettings;
    String j;
    j.reserve(200);
    j  = F("{\"ledMode\":");    j += gs.ledMode;
    j += F(",\"tempoLed\":");   j += gs.tempoLedEnabled ? F("true") : F("false");
    j += F(",\"neoPixel\":");   j += gs.neoPixelEnabled ? F("true") : F("false");
    j += F(",\"brightness\":"); j += gs.displayBrightness;
    j += F(",\"timeoutIdx\":"); j += gs.displayTimeoutIdx;
    j += F(",\"pot1CC\":");     j += (gs.pot1CC == POT_CC_OFF) ? -1 : (int)gs.pot1CC;
    j += F(",\"pot2CC\":");     j += (gs.pot2CC == POT_CC_OFF) ? -1 : (int)gs.pot2CC;
    j += F(",\"expCC\":");      j += gs.expCC;
    j += F(",\"expWake\":");    j += gs.expWakesDisplay ? F("true") : F("false");
    j += F(",\"routing\":");    j += gs.routingFlags;
    j += '}';
    return j;
}

inline void handleGetGlobal() {
    addCORS();
    webServer.send(200, F("application/json"), buildGlobalJson());
}

inline void handlePostGlobal() {
    addCORS();
    if(!_webPedal) { webServer.send(500); return; }
    GlobalSettings &gs = _webPedal->globalSettings;
    const String &body = webServer.arg("plain");
    int v; bool b;
    v = jsonInt(body, "ledMode");
    if(v >= 0 && v <= 2) gs.ledMode = (uint8_t)v;
    if(jsonBool(body, "tempoLed", b)) gs.tempoLedEnabled = b;
    if(jsonBool(body, "neoPixel", b)) gs.neoPixelEnabled = b;
    v = jsonInt(body, "brightness");
    if(v >= 0 && v <= 100) gs.displayBrightness = (uint8_t)v;
    v = jsonInt(body, "timeoutIdx");
    if(v >= 0 && v < NUM_DISP_TIMEOUTS) gs.displayTimeoutIdx = (uint8_t)v;
    v = jsonInt(body, "pot1CC");
    if(v == -1)                  { gs.pot1CC = POT_CC_OFF; analogPots[0].midiCCNumber = POT_CC_OFF; }
    else if(v >= 0 && v <= 127)  { gs.pot1CC = (uint8_t)v; analogPots[0].midiCCNumber = gs.pot1CC; }
    v = jsonInt(body, "pot2CC");
    if(v == -1)                  { gs.pot2CC = POT_CC_OFF; analogPots[1].midiCCNumber = POT_CC_OFF; }
    else if(v >= 0 && v <= 127)  { gs.pot2CC = (uint8_t)v; analogPots[1].midiCCNumber = gs.pot2CC; }
    v = jsonInt(body, "expCC");
    if(v >= 0 && v <= 127) gs.expCC = (uint8_t)v;
    if(jsonBool(body, "expWake", b)) gs.expWakesDisplay = b;
    v = jsonInt(body, "routing");
    if(v >= 0 && v <= (int)ROUTE_ALL) gs.routingFlags = (uint8_t)v;
    saveGlobalSettings(*_webPedal);
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
    triggerPresetSaveBlink();
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePostBpm() {
    addCORS();
    int b = jsonInt(webServer.arg("plain"), "bpm");
    if(b < (int)BPM_MIN || b > (int)BPM_MAX) {
        webServer.send(400, F("application/json"), F("{\"error\":\"bad bpm\"}"));
        return;
    }
    midiClock.setBpm((float)b);
    midiClock.externalSync = false;   // manual BPM overrides external sync
    webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline String buildPollJson() {
    String j;
    j.reserve(120);
    j  = F("{\"bpm\":");
    j += (int)midiClock.bpm;
    j += F(",\"externalSync\":");
    j += midiClock.externalSync ? F("true") : F("false");
    j += F(",\"activePreset\":");
    j += activePreset;
    j += F(",\"presetDirty\":");
    j += presetDirty ? F("true") : F("false");
    j += '}';
    return j;
}

inline void handleGetPoll() {
    addCORS();
    webServer.send(200, F("application/json"), buildPollJson());
}

inline void handleGetExpCal() {
    addCORS();
    unsigned long elapsed = expCalibrating ? (millis() - expCalStart) : (unsigned long)EXP_CAL_DURATION_MS;
    if (elapsed > EXP_CAL_DURATION_MS) elapsed = EXP_CAL_DURATION_MS;
    uint8_t secsLeft = expCalibrating
        ? (uint8_t)((EXP_CAL_DURATION_MS - elapsed + 999) / 1000)
        : 0;
    String j = F("{\"calibrating\":");
    j += expCalibrating ? F("true") : F("false");
    j += F(",\"secsLeft\":");
    j += secsLeft;
    j += '}';
    webServer.send(200, "application/json", j);
}

inline void handlePostExpCal() {
    addCORS();
    startExpCalibration();
    webServer.send(200, "application/json", F("{\"ok\":true}"));
}

// ── Init ───────────────────────────────────────────────────────────────────────
inline void initWebServer(PedalState &pedal) {
    _webPedal = &pedal;

    webServer.on("/",              HTTP_GET,  handleRoot);
    webServer.on("/api/state",     HTTP_GET,  handleGetState);
    webServer.on("/api/presets",   HTTP_GET,  handleGetPresets);
    webServer.on("/api/presets",   HTTP_OPTIONS, handleOPTIONS);
    webServer.on("/api/channel",   HTTP_POST, handlePostChannel);
    webServer.on("/api/channel",   HTTP_OPTIONS, handleOPTIONS);
    webServer.on("/api/bpm",       HTTP_POST, handlePostBpm);
    webServer.on("/api/bpm",       HTTP_OPTIONS, handleOPTIONS);
    webServer.on("/api/poll",      HTTP_GET,  handleGetPoll);
    webServer.on("/api/poll",      HTTP_OPTIONS, handleOPTIONS);
    webServer.on("/api/pot",       HTTP_POST, handlePostPot);
    webServer.on("/api/pot",       HTTP_OPTIONS, handleOPTIONS);
    webServer.on("/api/global",    HTTP_GET,  handleGetGlobal);
    webServer.on("/api/global",    HTTP_POST, handlePostGlobal);
    webServer.on("/api/global",    HTTP_OPTIONS, handleOPTIONS);
    webServer.on("/api/expcal",    HTTP_GET,  handleGetExpCal);
    webServer.on("/api/expcal",    HTTP_POST, handlePostExpCal);
    webServer.on("/api/expcal",    HTTP_OPTIONS, handleOPTIONS);

    webServer.on("/dismiss", HTTP_GET, handleDismiss);

    // Captive-portal catch-all — any unknown path redirects to the UI root.
    webServer.onNotFound(handleCaptivePortal);

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

    if(_apRunning) {
        dnsServer.processNextRequest();
        webServer.handleClient();
    }
}
