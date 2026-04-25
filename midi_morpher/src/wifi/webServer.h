#pragma once
#include "../analogInOut/expInput.h"
#include "../clock/midiClock.h"
#include "../controls/pots.h"
#include "../footswitches/footswitchObject.h"
#include "../pedalState.h"
#include "../statePersistance.h"
#include "webUI.h"
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

// ── AP credentials ─────────────────────────────────────────────────────────────
static constexpr char WIFI_AP_SSID[] = "MIDI Morpher";
static constexpr char WIFI_AP_PASS[] = "midimorpher";

// ── Module state ───────────────────────────────────────────────────────────────
inline WebServer webServer(80);
inline DNSServer dnsServer;
inline PedalState *_webPedal = nullptr;
inline bool _apRunning = false;
inline bool _dnsRunning = false;
inline bool _wasLocked = false;
inline bool _pendingRestart = false;

// ── OTA state ──────────────────────────────────────────────────────────────────
inline bool _otaError = false;
inline bool _otaStarted = false;
inline String _otaErrorMsg;

// AP gateway / captive-portal target address.
static const IPAddress AP_IP(192, 168, 4, 1);

// ── AP control ─────────────────────────────────────────────────────────────────
inline void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
  bool cp = !_webPedal || _webPedal->globalSettings.captivePortal;
  if(cp) {
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", AP_IP);
  }
  _dnsRunning = cp;
  MDNS.begin("midimorpher");
  webServer.begin();
  _apRunning = true;
}

inline void stopAP() {
  MDNS.end();
  if(_dnsRunning) {
    dnsServer.stop();
    _dnsRunning = false;
  }
  webServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  _apRunning = false;
}

// ── JSON helpers ───────────────────────────────────────────────────────────────
inline int jsonInt(const String &body, const char *key) {
  String k = String('"') + key + '"';
  int pos = body.indexOf(k);
  if(pos < 0)
    return -9999;
  pos = body.indexOf(':', pos + k.length());
  if(pos < 0)
    return -9999;
  pos++;
  while(pos < (int)body.length() && body[pos] == ' ')
    pos++;
  bool neg = false;
  if(pos < (int)body.length() && body[pos] == '-') {
    neg = true;
    pos++;
  }
  String num;
  while(pos < (int)body.length() && isDigit(body[pos]))
    num += body[pos++];
  if(num.isEmpty())
    return -9999;
  int v = num.toInt();
  return neg ? -v : v;
}

// Parse an unsigned 32-bit int. Returns 0xFFFFFFFF if the key is absent or
// unparseable. Needed for rampUpMs/rampDownMs which may have bit 31 set
// (CLOCK_SYNC_FLAG) and therefore overflow a plain signed int.
inline uint32_t jsonUint(const String &body, const char *key) {
  String k = String('"') + key + '"';
  int pos = body.indexOf(k);
  if(pos < 0)
    return 0xFFFFFFFFUL;
  pos = body.indexOf(':', pos + k.length());
  if(pos < 0)
    return 0xFFFFFFFFUL;
  pos++;
  while(pos < (int)body.length() && body[pos] == ' ')
    pos++;
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
  int pos = body.indexOf(k);
  if(pos < 0)
    return false;
  pos = body.indexOf(':', pos + k.length());
  if(pos < 0)
    return false;
  pos++;
  while(pos < (int)body.length() && body[pos] == ' ')
    pos++;
  if(body.substring(pos, pos + 4) == "true") {
    out = true;
    return true;
  }
  if(body.substring(pos, pos + 5) == "false") {
    out = false;
    return true;
  }
  return false;
}

// Find matching closing brace/bracket for the open character at pos.
inline int skipToMatch(const String &s, int pos) {
  char open = s[pos], close = (open == '{') ? '}' : ']';
  int depth = 0;
  bool inStr = false;
  for(int i = pos; i < (int)s.length(); i++) {
    char c = s[i];
    if(inStr) {
      if(c == '\\')
        i++;
      else if(c == '"')
        inStr = false;
      continue;
    }
    if(c == '"') {
      inStr = true;
      continue;
    }
    if(c == open)
      depth++;
    if(c == close) {
      if(--depth == 0)
        return i;
    }
  }
  return -1;
}

inline float jsonFloat(const String &body, const char *key) {
  String k = String('"') + key + '"';
  int pos = body.indexOf(k);
  if(pos < 0)
    return -1.0f;
  pos = body.indexOf(':', pos + k.length());
  if(pos < 0)
    return -1.0f;
  pos++;
  while(pos < (int)body.length() && body[pos] == ' ')
    pos++;
  String num;
  while(pos < (int)body.length() && (isDigit(body[pos]) || body[pos] == '.'))
    num += body[pos++];
  if(num.isEmpty())
    return -1.0f;
  return num.toFloat();
}

// ── State JSON builders ────────────────────────────────────────────────────────

inline String buildStateJson() {
  if(!_webPedal)
    return F("{}");
  const PedalState &p = *_webPedal;
  String j;
  j.reserve(3500);
  j = F("{\"channel\":");
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
    if(i)
      j += ',';
    bool upSync = (b.rampUpMs & CLOCK_SYNC_FLAG) != 0;
    bool dnSync = (b.rampDownMs & CLOCK_SYNC_FLAG) != 0;
    j += F("{\"id\":");
    j += i;
    j += F(",\"name\":\"");
    j += b.name;
    j += '"';
    j += F(",\"modeIndex\":");
    j += b.modeIndex;
    j += F(",\"modeName\":\"");
    j += modes[b.modeIndex].name;
    j += '"';
    j += F(",\"midiNumber\":");
    j += b.midiNumber;
    j += F(",\"fsChannel\":");
    j += b.fsChannel;
    j += F(",\"rampUpMs\":");
    j += upSync ? 0 : (uint32_t)b.rampUpMs;
    j += F(",\"rampDownMs\":");
    j += dnSync ? 0 : (uint32_t)b.rampDownMs;
    j += F(",\"rampUpSync\":");
    j += upSync ? F("true") : F("false");
    j += F(",\"rampUpNoteIdx\":");
    j += (uint8_t)(b.rampUpMs & 0xFF);
    j += F(",\"rampDownSync\":");
    j += dnSync ? F("true") : F("false");
    j += F(",\"rampDownNoteIdx\":");
    j += (uint8_t)(b.rampDownMs & 0xFF);
    j += F(",\"isModSwitch\":");
    j += b.isModSwitch ? F("true") : F("false");
    j += F(",\"isLatching\":");
    j += b.isLatching ? F("true") : F("false");
    j += F(",\"isKeyboard\":");
    j += b.isKeyboard ? F("true") : F("false");
    j += F(",\"ccLow\":");
    j += b.ccLow;
    j += F(",\"ccHigh\":");
    j += b.ccHigh;
    j += F(",\"extraActions\":[");
    for(int t = 0; t < (int)FS_NUM_EXTRA; t++) {
      if(t)
        j += ',';
      const FSAction &act = b.extraActions[t];
      bool upS = (act.rampUpMs & CLOCK_SYNC_FLAG) != 0;
      bool dnS = (act.rampDownMs & CLOCK_SYNC_FLAG) != 0;
      j += F("{\"enabled\":");
      j += act.enabled ? F("true") : F("false");
      j += F(",\"modeIndex\":");
      j += act.modeIndex;
      j += F(",\"midiNumber\":");
      j += act.midiNumber;
      j += F(",\"fsChannel\":");
      j += act.fsChannel;
      j += F(",\"ccLow\":");
      j += act.ccLow;
      j += F(",\"ccHigh\":");
      j += act.ccHigh;
      j += F(",\"rampUpMs\":");
      j += upS ? 0u : (uint32_t)act.rampUpMs;
      j += F(",\"rampDownMs\":");
      j += dnS ? 0u : (uint32_t)act.rampDownMs;
      j += F(",\"rampUpSync\":");
      j += upS ? F("true") : F("false");
      j += F(",\"rampUpNoteIdx\":");
      j += (uint8_t)(act.rampUpMs & 0xFF);
      j += F(",\"rampDownSync\":");
      j += dnS ? F("true") : F("false");
      j += F(",\"rampDownNoteIdx\":");
      j += (uint8_t)(act.rampDownMs & 0xFF);
      j += '}';
    }
    j += F("]");
    j += '}';
  }
  j += F("]}");
  return j;
}

inline String buildPresetsJson() {
  String j;
  j.reserve(2800);
  j = F("{\"activePreset\":");
  j += activePreset;
  j += F(",\"presets\":[");
  for(int p = 0; p < NUM_PRESETS; p++) {
    if(p)
      j += ',';
    j += F("{\"midiChannel\":");
    j += presets[p].midiChannel;
    j += F(",\"buttons\":[");
    for(int i = 0; i < 6; i++) {
      if(i)
        j += ',';
      const FSButtonPersisted &b = presets[p].buttons[i];
      j += F("{\"modeIndex\":");
      j += b.modeIndex;
      j += F(",\"midiNumber\":");
      j += b.midiNumber;
      j += F(",\"rampUpMs\":");
      j += b.rampUpMs;
      j += F(",\"rampDownMs\":");
      j += b.rampDownMs;
      j += F(",\"fsChannel\":");
      j += b.fsChannel;
      j += '}';
    }
    j += F("]}");
  }
  j += F("]}");
  return j;
}

inline String buildBackupJson() {
  if(!_webPedal)
    return F("{}");
  const GlobalSettings &gs = _webPedal->globalSettings;
  String j;
  j.reserve(4500);
  j = F("{\"version\":1,\"activePreset\":");
  j += activePreset;
  j += F(",\"globalSettings\":{");
  j += F("\"ledMode\":");
  j += gs.ledMode;
  j += F(",\"tempoLedEnabled\":");
  j += gs.tempoLedEnabled ? F("true") : F("false");
  j += F(",\"neoPixelEnabled\":");
  j += gs.neoPixelEnabled ? F("true") : F("false");
  j += F(",\"displayBrightness\":");
  j += gs.displayBrightness;
  j += F(",\"displayTimeoutIdx\":");
  j += gs.displayTimeoutIdx;
  j += F(",\"pot1CC\":");
  j += (gs.pot1CC == POT_CC_OFF) ? -1 : (int)gs.pot1CC;
  j += F(",\"pot2CC\":");
  j += (gs.pot2CC == POT_CC_OFF) ? -1 : (int)gs.pot2CC;
  j += F(",\"expCC\":");
  j += (gs.expCC == POT_CC_OFF) ? -1 : (int)gs.expCC;
  j += F(",\"expWakesDisplay\":");
  j += gs.expWakesDisplay ? F("true") : F("false");
  j += F(",\"routingFlags\":");
  j += gs.routingFlags;
  j += F(",\"expCalMin\":");
  j += gs.expCalMin;
  j += F(",\"expCalMax\":");
  j += gs.expCalMax;
  j += F(",\"perFsModulator\":");
  j += gs.perFsModulator ? F("true") : F("false");
  j += F(",\"clockGenerate\":");
  j += gs.clockGenerate ? F("true") : F("false");
  j += F(",\"clockOutput\":");
  j += gs.clockOutput ? F("true") : F("false");
  j += F(",\"captivePortal\":");
  j += gs.captivePortal ? F("true") : F("false");
  j += F("},\"presets\":[");
  for(int p = 0; p < NUM_PRESETS; p++) {
    if(p)
      j += ',';
    const PresetData &pd = presets[p];
    j += F("{\"midiChannel\":");
    j += pd.midiChannel;
    j += F(",\"bpm\":");
    j += (int)pd.bpm;
    j += F(",\"buttons\":[");
    for(int i = 0; i < 6; i++) {
      if(i)
        j += ',';
      const FSButtonPersisted &b = pd.buttons[i];
      j += F("{\"modeIndex\":");
      j += b.modeIndex;
      j += F(",\"midiNumber\":");
      j += b.midiNumber;
      j += F(",\"rampUpMs\":");
      j += b.rampUpMs;
      j += F(",\"rampDownMs\":");
      j += b.rampDownMs;
      j += F(",\"fsChannel\":");
      j += b.fsChannel;
      j += F(",\"ccLow\":");
      j += b.ccLow;
      j += F(",\"ccHigh\":");
      j += b.ccHigh;
      j += '}';
    }
    j += F("]}");
  }
  j += F("],\"multis\":[");
  bool mfirst = true;
  for(uint8_t i = 0; i < MAX_MULTI_SCENES; i++) {
    if(multiScenes[i].name[0] == '\0')
      continue;
    if(!mfirst)
      j += ',';
    mfirst = false;
    j += F("{\"id\":");
    j += i;
    j += F(",\"name\":\"");
    j += multiScenes[i].name;
    j += '"';
    j += F(",\"count\":");
    j += multiScenes[i].count;
    j += F(",\"subCmds\":[");
    for(uint8_t s = 0; s < multiScenes[i].count && s < MAX_MULTI_SUBCMDS; s++) {
      const MultiSubCmd &sc = multiScenes[i].subCmds[s];
      if(s)
        j += ',';
      j += F("{\"modeIndex\":");
      j += sc.modeIndex;
      j += F(",\"midiNumber\":");
      j += sc.midiNumber;
      j += F(",\"channel\":");
      j += sc.channel;
      j += '}';
    }
    j += F("]}");
  }
  j += F("]}");
  return j;
}

// ── CORS helper ────────────────────────────────────────────────────────────────
inline void addCORS() {
  webServer.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
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

// Captive-portal catch-all. Any URL other than the registered ones — including
// OS probes like /generate_204, /hotspot-detect.html, /ncsi.txt — gets a 302
// redirect to the root, which triggers the "Sign in to network" prompt on the
// phone/laptop and takes the user straight to the UI.
inline void handleCaptivePortal() {
  if(!_webPedal || !_webPedal->globalSettings.captivePortal) {
    webServer.send(404, F("text/plain"), F("Not found"));
    return;
  }
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
  if(!_webPedal) {
    webServer.send(500);
    return;
  }
  int ch = jsonInt(webServer.arg("plain"), "channel");
  if(ch < 0 || ch > 15) {
    webServer.send(400, F("application/json"), F("{\"error\":\"bad channel\"}"));
    return;
  }
  _webPedal->setMidiChannel((uint8_t)ch);
  markStateDirty();
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

// Ramp value accepts either plain ms (0–5000) or CLOCK_SYNC_FLAG | noteIdx.
// Returns true if the value is a valid ramp.
inline bool _validRamp(uint32_t v) {
  if(v == 0xFFFFFFFFUL)
    return false;
  if(v & CLOCK_SYNC_FLAG)
    return (v & 0xFF) < NUM_NOTE_VALUES;
  return v <= 5000;
}

inline void handlePostButton(int idx) {
  addCORS();
  if(!_webPedal) {
    webServer.send(500);
    return;
  }
  if(idx < 0 || idx >= (int)_webPedal->buttons.size()) {
    webServer.send(400);
    return;
  }
  FSButton &btn = _webPedal->buttons[idx];
  const String &body = webServer.arg("plain");
  int mi = jsonInt(body, "modeIndex");
  int mn = jsonInt(body, "midiNumber");
  int ch = jsonInt(body, "fsChannel");
  uint32_t up = jsonUint(body, "rampUpMs");
  uint32_t dn = jsonUint(body, "rampDownMs");
  if(mi >= 0 && mi < NUM_MODES)
    applyModeIndex(btn, (uint8_t)mi, &_webPedal->modForFS(idx));
  // Clamp midiNumber against mode-appropriate max. Scene modes cap at
  // sceneMaxVal (7 for Helix/QC/Fractal, 4 for Kemper); system commands cap
  // at NUM_SYS_CMDS-1; modulation modes additionally accept 255 (PB_SENTINEL)
  // meaning Pitch Bend destination; everything else 0–127.
  if(btn.isModSwitch && mn == (int)PB_SENTINEL) {
    btn.midiNumber = PB_SENTINEL;
  } else {
    int mnMax = btn.isScene    ? (int)btn.modMode.sceneMaxVal
                : btn.isSystem ? (int)(NUM_SYS_CMDS - 1)
                               : 127;
    if(mn >= 0 && mn <= mnMax)
      btn.midiNumber = (uint8_t)mn;
  }
  if(ch == 255 || (ch >= 0 && ch <= 15))
    btn.fsChannel = (uint8_t)ch;
  if(_validRamp(up))
    btn.rampUpMs = up;
  if(_validRamp(dn))
    btn.rampDownMs = dn;
  int cl = jsonInt(body, "ccLow");
  int ch2 = jsonInt(body, "ccHigh");
  if(cl >= 0 && cl <= 127)
    btn.ccLow = (uint8_t)cl;
  if(ch2 >= 0 && ch2 <= 127)
    btn.ccHigh = (uint8_t)ch2;
  markStateDirty();
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handleButtonPress(int idx) {
  addCORS();
  if(!_webPedal || idx < 0 || idx >= 6) {
    webServer.send(400);
    return;
  }
  FSButton &btn = _webPedal->buttons[idx];
  btn.simulatePress(true, _webPedal->midiChannel, _webPedal->modForFS(idx));
  _webPedal->lastActiveFSIndex = idx;
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handleButtonRelease(int idx) {
  addCORS();
  if(!_webPedal || idx < 0 || idx >= 6) {
    webServer.send(400);
    return;
  }
  FSButton &btn = _webPedal->buttons[idx];
  btn.simulatePress(false, _webPedal->midiChannel, _webPedal->modForFS(idx));
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePostButtonAction(int idx) {
  addCORS();
  if(!_webPedal || idx < 0 || idx >= 6) {
    webServer.send(400);
    return;
  }
  FSButton &btn = _webPedal->buttons[idx];
  const String &body = webServer.arg("plain");
  int t = jsonInt(body, "type");
  if(t < 0 || t >= (int)FS_NUM_EXTRA) {
    webServer.send(400);
    return;
  }
  FSAction &act = btn.extraActions[t];
  bool enabled;
  if(!jsonBool(body, "enabled", enabled))
    enabled = true;
  act.enabled = enabled;
  if(enabled) {
    int mi = jsonInt(body, "modeIndex");
    int mn = jsonInt(body, "midiNumber");
    int ch = jsonInt(body, "fsChannel");
    uint32_t up = jsonUint(body, "rampUpMs");
    uint32_t dn = jsonUint(body, "rampDownMs");
    int cl = jsonInt(body, "ccLow");
    int cH = jsonInt(body, "ccHigh");
    if(mi >= 0 && mi < NUM_MODES)
      act.modeIndex = (uint8_t)mi;
    const ModeInfo &minfo = modes[act.modeIndex];
    int mnMax = minfo.isScene    ? (int)minfo.sceneMaxVal
                : minfo.isSystem ? (int)(NUM_SYS_CMDS - 1)
                                 : 127;
    if(mn >= 0 && mn <= mnMax)
      act.midiNumber = (uint8_t)mn;
    if(ch == 255 || (ch >= 0 && ch <= 15))
      act.fsChannel = (uint8_t)ch;
    if(_validRamp(up))
      act.rampUpMs = up;
    if(_validRamp(dn))
      act.rampDownMs = dn;
    if(cl >= 0 && cl <= 127)
      act.ccLow = (uint8_t)cl;
    if(cH >= 0 && cH <= 127)
      act.ccHigh = (uint8_t)cH;
  }
  markStateDirty();
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePostPot() {
  addCORS();
  if(!_webPedal) {
    webServer.send(500);
    return;
  }
  const String &body = webServer.arg("plain");
  int id = jsonInt(body, "id");
  int value = jsonInt(body, "value");
  if(id < 0 || id > 1 || value < 0 || value > 127) {
    webServer.send(400);
    return;
  }
  uint8_t cc = (id == 0) ? _webPedal->globalSettings.pot1CC
                         : _webPedal->globalSettings.pot2CC;
  if(cc != POT_CC_OFF)
    sendMIDI(_webPedal->midiChannel, false, cc, (uint8_t)value);
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

// ── Global settings ────────────────────────────────────────────────────────────

inline String buildGlobalJson() {
  if(!_webPedal)
    return F("{}");
  const GlobalSettings &gs = _webPedal->globalSettings;
  String j;
  j.reserve(200);
  j = F("{\"ledMode\":");
  j += gs.ledMode;
  j += F(",\"tempoLed\":");
  j += gs.tempoLedEnabled ? F("true") : F("false");
  j += F(",\"neoPixel\":");
  j += gs.neoPixelEnabled ? F("true") : F("false");
  j += F(",\"brightness\":");
  j += gs.displayBrightness;
  j += F(",\"timeoutIdx\":");
  j += gs.displayTimeoutIdx;
  j += F(",\"pot1CC\":");
  j += (gs.pot1CC == POT_CC_OFF) ? -1 : (int)gs.pot1CC;
  j += F(",\"pot2CC\":");
  j += (gs.pot2CC == POT_CC_OFF) ? -1 : (int)gs.pot2CC;
  j += F(",\"expCC\":");
  j += (gs.expCC == POT_CC_OFF) ? -1 : (int)gs.expCC;
  j += F(",\"expWake\":");
  j += gs.expWakesDisplay ? F("true") : F("false");
  j += F(",\"routing\":");
  j += gs.routingFlags;
  j += F(",\"perFsMod\":");
  j += gs.perFsModulator ? F("true") : F("false");
  j += F(",\"clockGen\":");
  j += gs.clockGenerate ? F("true") : F("false");
  j += F(",\"clockOut\":");
  j += gs.clockOutput ? F("true") : F("false");
  j += F(",\"captivePortal\":");
  j += gs.captivePortal ? F("true") : F("false");
  j += '}';
  return j;
}

inline void handleGetGlobal() {
  addCORS();
  webServer.send(200, F("application/json"), buildGlobalJson());
}

inline void handlePostGlobal() {
  addCORS();
  if(!_webPedal) {
    webServer.send(500);
    return;
  }
  GlobalSettings &gs = _webPedal->globalSettings;
  const String &body = webServer.arg("plain");
  int v;
  bool b;
  v = jsonInt(body, "ledMode");
  if(v >= 0 && v <= 2)
    gs.ledMode = (uint8_t)v;
  if(jsonBool(body, "tempoLed", b))
    gs.tempoLedEnabled = b;
  if(jsonBool(body, "neoPixel", b))
    gs.neoPixelEnabled = b;
  v = jsonInt(body, "brightness");
  if(v >= 0 && v <= 100)
    gs.displayBrightness = (uint8_t)v;
  v = jsonInt(body, "timeoutIdx");
  if(v >= 0 && v < NUM_DISP_TIMEOUTS)
    gs.displayTimeoutIdx = (uint8_t)v;
  v = jsonInt(body, "pot1CC");
  if(v == -1) {
    gs.pot1CC = POT_CC_OFF;
    analogPots[0].midiCCNumber = POT_CC_OFF;
  } else if(v >= 0 && v <= 127) {
    gs.pot1CC = (uint8_t)v;
    analogPots[0].midiCCNumber = gs.pot1CC;
  }
  v = jsonInt(body, "pot2CC");
  if(v == -1) {
    gs.pot2CC = POT_CC_OFF;
    analogPots[1].midiCCNumber = POT_CC_OFF;
  } else if(v >= 0 && v <= 127) {
    gs.pot2CC = (uint8_t)v;
    analogPots[1].midiCCNumber = gs.pot2CC;
  }
  v = jsonInt(body, "expCC");
  if(v >= 0 && v <= 127)
    gs.expCC = (uint8_t)v;
  if(jsonBool(body, "expWake", b))
    gs.expWakesDisplay = b;
  v = jsonInt(body, "routing");
  if(v >= 0 && v <= (int)ROUTE_ALL)
    gs.routingFlags = (uint8_t)v;
  if(jsonBool(body, "perFsMod", b))
    gs.perFsModulator = b;
  if(jsonBool(body, "clockGen", b))
    gs.clockGenerate = b;
  if(jsonBool(body, "clockOut", b))
    gs.clockOutput = b;
  if(jsonBool(body, "captivePortal", b)) {
    if(b != gs.captivePortal) {
      gs.captivePortal = b;
      if(_apRunning) {
        if(b && !_dnsRunning) {
          dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
          dnsServer.start(53, "*", AP_IP);
          _dnsRunning = true;
        } else if(!b && _dnsRunning) {
          dnsServer.stop();
          _dnsRunning = false;
        }
      }
    }
  }
  saveGlobalSettings(*_webPedal);
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePresetLoad(int idx) {
  addCORS();
  if(!_webPedal || idx < 0 || idx >= NUM_PRESETS) {
    webServer.send(400);
    return;
  }
  applyPreset((uint8_t)idx, *_webPedal);
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void handlePresetSave(int idx) {
  addCORS();
  if(!_webPedal || idx < 0 || idx >= NUM_PRESETS) {
    webServer.send(400);
    return;
  }
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
  midiClock.externalSync = false; // manual BPM overrides external sync
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline String buildPollJson() {
  String j;
  j.reserve(120);
  j = F("{\"bpm\":");
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
  if(elapsed > EXP_CAL_DURATION_MS)
    elapsed = EXP_CAL_DURATION_MS;
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

// ── Backup / Restore / Factory Reset ──────────────────────────────────────────

inline void handleGetBackup() {
  addCORS();
  webServer.send(200, F("application/json"), buildBackupJson());
}

inline void handlePostRestore() {
  addCORS();
  if(!_webPedal) {
    webServer.send(500);
    return;
  }
  if(_webPedal->settingsLocked) {
    webServer.send(403, F("application/json"), F("{\"error\":\"locked\"}"));
    return;
  }
  const String &body = webServer.arg("plain");
  if(jsonInt(body, "version") != 1) {
    webServer.send(400, F("application/json"), F("{\"error\":\"bad version\"}"));
    return;
  }
  int newAp = jsonInt(body, "activePreset");

  // Global settings
  int gsPos = body.indexOf(F("\"globalSettings\":"));
  if(gsPos >= 0) {
    int oPos = body.indexOf('{', gsPos + 17);
    if(oPos >= 0) {
      int oEnd = skipToMatch(body, oPos);
      if(oEnd > oPos) {
        String gs = body.substring(oPos, oEnd + 1);
        GlobalSettings &g = _webPedal->globalSettings;
        int v;
        bool b;
        v = jsonInt(gs, "ledMode");
        if(v >= 0 && v <= 2)
          g.ledMode = (uint8_t)v;
        if(jsonBool(gs, "tempoLedEnabled", b))
          g.tempoLedEnabled = b;
        if(jsonBool(gs, "neoPixelEnabled", b))
          g.neoPixelEnabled = b;
        v = jsonInt(gs, "displayBrightness");
        if(v >= 0 && v <= 100)
          g.displayBrightness = (uint8_t)v;
        v = jsonInt(gs, "displayTimeoutIdx");
        if(v >= 0 && v < NUM_DISP_TIMEOUTS)
          g.displayTimeoutIdx = (uint8_t)v;
        v = jsonInt(gs, "pot1CC");
        if(v == -1) {
          g.pot1CC = POT_CC_OFF;
          analogPots[0].midiCCNumber = POT_CC_OFF;
        } else if(v >= 0 && v <= 127) {
          g.pot1CC = (uint8_t)v;
          analogPots[0].midiCCNumber = g.pot1CC;
        }
        v = jsonInt(gs, "pot2CC");
        if(v == -1) {
          g.pot2CC = POT_CC_OFF;
          analogPots[1].midiCCNumber = POT_CC_OFF;
        } else if(v >= 0 && v <= 127) {
          g.pot2CC = (uint8_t)v;
          analogPots[1].midiCCNumber = g.pot2CC;
        }
        v = jsonInt(gs, "expCC");
        if(v >= 0 && v <= 127)
          g.expCC = (uint8_t)v;
        if(jsonBool(gs, "expWakesDisplay", b))
          g.expWakesDisplay = b;
        v = jsonInt(gs, "routingFlags");
        if(v >= 0 && v <= (int)ROUTE_ALL)
          g.routingFlags = (uint8_t)v;
        uint32_t ecMin = jsonUint(gs, "expCalMin");
        uint32_t ecMax = jsonUint(gs, "expCalMax");
        if(ecMin != 0xFFFFFFFFUL && ecMin <= 0xFFFF)
          g.expCalMin = (uint16_t)ecMin;
        if(ecMax != 0xFFFFFFFFUL && ecMax <= 0xFFFF)
          g.expCalMax = (uint16_t)ecMax;
        if(jsonBool(gs, "perFsModulator", b))
          g.perFsModulator = b;
        if(jsonBool(gs, "clockGenerate", b))
          g.clockGenerate = b;
        if(jsonBool(gs, "clockOutput", b))
          g.clockOutput = b;
        if(jsonBool(gs, "captivePortal", b))
          g.captivePortal = b;
        saveGlobalSettings(*_webPedal);
      }
    }
  }

  // Presets
  int prPos = body.indexOf(F("\"presets\":"));
  if(prPos >= 0) {
    int arrPos = body.indexOf('[', prPos + 10);
    if(arrPos >= 0) {
      int arrEnd = skipToMatch(body, arrPos);
      if(arrEnd > arrPos) {
        int pos = arrPos + 1;
        for(int p = 0; p < NUM_PRESETS && pos < arrEnd; p++) {
          while(pos < arrEnd && body[pos] != '{')
            pos++;
          if(pos >= arrEnd)
            break;
          int objEnd = skipToMatch(body, pos);
          if(objEnd < 0)
            break;
          String pStr = body.substring(pos, objEnd + 1);
          int ch = jsonInt(pStr, "midiChannel");
          float bpm = jsonFloat(pStr, "bpm");
          if(ch >= 0 && ch <= 15)
            presets[p].midiChannel = (uint8_t)ch;
          if(bpm >= BPM_MIN && bpm <= BPM_MAX)
            presets[p].bpm = bpm;
          int btPos = pStr.indexOf(F("\"buttons\":"));
          if(btPos >= 0) {
            int baPos = pStr.indexOf('[', btPos + 10);
            if(baPos >= 0) {
              int baEnd = skipToMatch(pStr, baPos);
              if(baEnd > baPos) {
                int bpos = baPos + 1;
                for(int bi = 0; bi < 6 && bpos < baEnd; bi++) {
                  while(bpos < baEnd && pStr[bpos] != '{')
                    bpos++;
                  if(bpos >= baEnd)
                    break;
                  int bend = skipToMatch(pStr, bpos);
                  if(bend < 0)
                    break;
                  String bStr = pStr.substring(bpos, bend + 1);
                  FSButtonPersisted &btn = presets[p].buttons[bi];
                  int mi = jsonInt(bStr, "modeIndex");
                  int mn = jsonInt(bStr, "midiNumber");
                  uint32_t ru = jsonUint(bStr, "rampUpMs");
                  uint32_t rd = jsonUint(bStr, "rampDownMs");
                  int fsc = jsonInt(bStr, "fsChannel");
                  int cl = jsonInt(bStr, "ccLow");
                  int cH = jsonInt(bStr, "ccHigh");
                  if(mi >= 0 && mi < NUM_MODES)
                    btn.modeIndex = (uint8_t)mi;
                  if(mn >= 0 && mn <= 255)
                    btn.midiNumber = (uint8_t)mn;
                  if(_validRamp(ru))
                    btn.rampUpMs = ru;
                  if(_validRamp(rd))
                    btn.rampDownMs = rd;
                  if(fsc == 255 || (fsc >= 0 && fsc <= 15))
                    btn.fsChannel = (uint8_t)fsc;
                  if(cl >= 0 && cl <= 127)
                    btn.ccLow = (uint8_t)cl;
                  if(cH >= 0 && cH <= 127)
                    btn.ccHigh = (uint8_t)cH;
                  bpos = bend + 1;
                }
              }
            }
          }
          pos = objEnd + 1;
        }
      }
    }
  }
  saveAllPresets();

  // Restore multi scenes
  int mlPos = body.indexOf(F("\"multis\":"));
  if(mlPos >= 0) {
    int arrPos = body.indexOf('[', mlPos + 9);
    if(arrPos >= 0) {
      int arrEnd = skipToMatch(body, arrPos);
      if(arrEnd > arrPos) {
        memset(multiScenes, 0, sizeof(multiScenes));
        int pos = arrPos + 1;
        while(pos < arrEnd) {
          while(pos < arrEnd && body[pos] != '{')
            pos++;
          if(pos >= arrEnd)
            break;
          int objEnd = skipToMatch(body, pos);
          if(objEnd < 0)
            break;
          String ms = body.substring(pos, objEnd + 1);
          int sid = jsonInt(ms, "id");
          if(sid >= 0 && sid < (int)MAX_MULTI_SCENES) {
            MultiScene &sc = multiScenes[sid];
            memset(&sc, 0, sizeof(sc));
            int np = ms.indexOf(F("\"name\":\""));
            if(np >= 0) {
              np += 8;
              int ne = ms.indexOf('"', np);
              if(ne > np) {
                int ln = min(ne - np, (int)(MULTI_NAME_LEN - 1));
                ms.substring(np, np + ln).toCharArray(sc.name, MULTI_NAME_LEN);
              }
            }
            int scPos = ms.indexOf(F("\"subCmds\":"));
            if(scPos >= 0) {
              int ap2 = ms.indexOf('[', scPos + 10);
              if(ap2 >= 0) {
                int ae2 = skipToMatch(ms, ap2);
                int bp = ap2 + 1;
                uint8_t cnt = 0;
                while(bp < ae2 && cnt < MAX_MULTI_SUBCMDS) {
                  while(bp < ae2 && ms[bp] != '{')
                    bp++;
                  if(bp >= ae2)
                    break;
                  int be = skipToMatch(ms, bp);
                  if(be < 0)
                    break;
                  String sub = ms.substring(bp, be + 1);
                  int mi = jsonInt(sub, "modeIndex");
                  int mn = jsonInt(sub, "midiNumber");
                  int ch = jsonInt(sub, "channel");
                  if(mi >= 0 && mi < (int)NUM_MODES) {
                    sc.subCmds[cnt].modeIndex = (uint8_t)mi;
                    sc.subCmds[cnt].midiNumber = (mn >= 0 && mn <= 127) ? (uint8_t)mn : 0;
                    sc.subCmds[cnt].channel = (ch == 255 || (ch >= 0 && ch <= 15))
                                                  ? (uint8_t)ch
                                                  : 0xFF;
                    cnt++;
                  }
                  bp = be + 1;
                }
                sc.count = cnt;
              }
            }
          }
          pos = objEnd + 1;
        }
        saveMultiScenes();
      }
    }
  }

  uint8_t ap = (newAp >= 0 && newAp < NUM_PRESETS) ? (uint8_t)newAp : 0;
  applyPreset(ap, *_webPedal);
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

inline void performFactoryReset(PedalState &state) {
  const FSButtonPersisted dflt = {0, 0, DEFAULT_RAMP_SPEED, DEFAULT_RAMP_SPEED, 0xFF, 0, 127, 0};
  // Presets 0, 3, 4, 5: PC mode, program numbers 0-5 (P1–P6)
  for(int p = 0; p < NUM_PRESETS; p++) {
    if(p == 1 || p == 2)
      continue;
    presets[p].midiChannel = 0;
    presets[p].bpm = DEFAULT_BPM;
    for(int i = 0; i < 6; i++) {
      presets[p].buttons[i] = dflt;
      presets[p].buttons[i].midiNumber = (uint8_t)i;
    }
  }
  // Preset 1 (index 1): modulation — Ramp, Ramp Latch, LFO Sine, LFO Sine L, Step Latch, Tap Tempo; all on CC1
  {
    presets[1].midiChannel = 0;
    presets[1].bpm = DEFAULT_BPM;
    const uint8_t mi[6] = {5, 6, 9, 10, 16, 31};
    const uint8_t mn[6] = {1, 1, 1, 1, 1, 0};
    for(int i = 0; i < 6; i++) {
      presets[1].buttons[i] = dflt;
      presets[1].buttons[i].modeIndex = mi[i];
      presets[1].buttons[i].midiNumber = mn[i];
    }
  }
  // Preset 2 (index 2): transport — Play, Stop, Record, Continue, Rewind, FFwd
  {
    presets[2].midiChannel = 0;
    presets[2].bpm = DEFAULT_BPM;
    const uint8_t sc[6] = {0, 1, 3, 2, 5, 6};
    for(int i = 0; i < 6; i++) {
      presets[2].buttons[i] = dflt;
      presets[2].buttons[i].modeIndex = 32;
      presets[2].buttons[i].midiNumber = sc[i];
    }
  }
  saveAllPresets();
  applyPreset(0, state);
}

inline void handlePostFactoryReset() {
  addCORS();
  if(!_webPedal) {
    webServer.send(500);
    return;
  }
  if(_webPedal->settingsLocked) {
    webServer.send(403, F("application/json"), F("{\"error\":\"locked\"}"));
    return;
  }
  performFactoryReset(*_webPedal);
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

// ── Multi scene API ────────────────────────────────────────────────────────────

inline String buildMultiScenesJson() {
  String j = "[";
  bool first = true;
  for(uint8_t i = 0; i < MAX_MULTI_SCENES; i++) {
    if(multiScenes[i].name[0] == '\0')
      continue;
    if(!first)
      j += ',';
    first = false;
    j += F("{\"id\":");
    j += i;
    j += F(",\"name\":\"");
    j += multiScenes[i].name;
    j += '"';
    j += F(",\"count\":");
    j += multiScenes[i].count;
    j += F(",\"subCmds\":[");
    for(uint8_t s = 0; s < multiScenes[i].count && s < MAX_MULTI_SUBCMDS; s++) {
      const MultiSubCmd &sc = multiScenes[i].subCmds[s];
      if(s)
        j += ',';
      j += F("{\"modeIndex\":");
      j += sc.modeIndex;
      j += F(",\"midiNumber\":");
      j += sc.midiNumber;
      j += F(",\"channel\":");
      j += sc.channel;
      j += '}';
    }
    j += F("]}");
  }
  j += ']';
  return j;
}

inline void handleGetMultiScenes() {
  addCORS();
  webServer.send(200, F("application/json"), buildMultiScenesJson());
}

inline void handlePostMultiScenes() {
  addCORS();
  if(!_webPedal) {
    webServer.send(500);
    return;
  }
  if(_webPedal->settingsLocked) {
    webServer.send(403, F("application/json"), F("{\"error\":\"locked\"}"));
    return;
  }
  uint8_t slot = findEmptyMultiSlot();
  if(slot == 0xFF) {
    webServer.send(400, F("application/json"), F("{\"error\":\"full\"}"));
    return;
  }
  const String &body = webServer.arg("plain");
  MultiScene &sc = multiScenes[slot];
  memset(&sc, 0, sizeof(sc));
  // Parse name
  int npos = body.indexOf(F("\"name\":\""));
  if(npos >= 0) {
    npos += 8;
    int end = body.indexOf('"', npos);
    if(end > npos) {
      int len = min(end - npos, (int)(MULTI_NAME_LEN - 1));
      body.substring(npos, npos + len).toCharArray(sc.name, MULTI_NAME_LEN);
    }
  }
  if(sc.name[0] == '\0')
    snprintf(sc.name, MULTI_NAME_LEN, "Multi %d", slot + 1);
  // Parse subCmds array
  int scPos = body.indexOf(F("\"subCmds\":"));
  if(scPos >= 0) {
    int arrPos = body.indexOf('[', scPos + 10);
    if(arrPos >= 0) {
      int arrEnd = skipToMatch(body, arrPos);
      int pos = arrPos + 1;
      uint8_t cnt = 0;
      while(pos < arrEnd && cnt < MAX_MULTI_SUBCMDS) {
        while(pos < arrEnd && body[pos] != '{')
          pos++;
        if(pos >= arrEnd)
          break;
        int objEnd = skipToMatch(body, pos);
        if(objEnd < 0)
          break;
        String sub = body.substring(pos, objEnd + 1);
        int mi = jsonInt(sub, "modeIndex");
        int mn = jsonInt(sub, "midiNumber");
        int ch = jsonInt(sub, "channel");
        if(mi >= 0 && mi < (int)NUM_MODES) {
          sc.subCmds[cnt].modeIndex = (uint8_t)mi;
          sc.subCmds[cnt].midiNumber = (mn >= 0 && mn <= 127) ? (uint8_t)mn : 0;
          sc.subCmds[cnt].channel = (ch == 255 || (ch >= 0 && ch <= 15))
                                        ? (uint8_t)ch
                                        : 0xFF;
          cnt++;
        }
        pos = objEnd + 1;
      }
      sc.count = cnt;
    }
  }
  saveMultiScenes();
  String resp = F("{\"ok\":true,\"id\":");
  resp += slot;
  resp += '}';
  webServer.send(200, F("application/json"), resp);
}

inline void handleDeleteMultiScene(int id) {
  addCORS();
  if(!_webPedal) {
    webServer.send(500);
    return;
  }
  if(_webPedal->settingsLocked) {
    webServer.send(403, F("application/json"), F("{\"error\":\"locked\"}"));
    return;
  }
  if(id < 0 || id >= (int)MAX_MULTI_SCENES) {
    webServer.send(400);
    return;
  }
  memset(&multiScenes[id], 0, sizeof(MultiScene));
  saveMultiScenes();
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
}

// ── OTA update ─────────────────────────────────────────────────────────────────

// Upload handler — called repeatedly as multipart chunks arrive.
// Checks the ESP32 firmware magic byte on the first chunk; feeds subsequent
// chunks to the Update library. Any error aborts the update and sets the flag
// so the response handler can report it without touching flash further.
inline void handleOTAUpload() {
  if(_webPedal && _webPedal->settingsLocked)
    return;
  HTTPUpload &upload = webServer.upload();

  if(upload.status == UPLOAD_FILE_START) {
    _otaError = false;
    _otaStarted = false;
    _otaErrorMsg = "";
    if(!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      _otaError = true;
      _otaErrorMsg = Update.errorString();
    }
  } else if(upload.status == UPLOAD_FILE_WRITE) {
    if(_otaError)
      return;
    // Validate magic byte on the very first data chunk before writing anything.
    if(!_otaStarted && upload.currentSize > 0) {
      _otaStarted = true;
      if(upload.buf[0] != 0xE9) {
        _otaError = true;
        _otaErrorMsg = "Not a firmware file (bad magic byte)";
        Update.abort();
        return;
      }
    }
    if(Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      _otaError = true;
      _otaErrorMsg = Update.errorString();
    }
  } else if(upload.status == UPLOAD_FILE_END) {
    if(!_otaError && !Update.end(true)) {
      _otaError = true;
      _otaErrorMsg = Update.errorString();
    }
  }
}

// Response handler — called once after the upload is fully received.
// On success, sends 200 and sets a flag so the main loop restarts after the
// TCP response has been flushed. On failure, the old firmware remains active.
inline void handleOTAResponse() {
  addCORS();
  if(_webPedal && _webPedal->settingsLocked) {
    webServer.send(403, F("application/json"), F("{\"error\":\"locked\"}"));
    return;
  }
  if(_otaError || Update.hasError()) {
    String r = F("{\"error\":\"");
    r += _otaErrorMsg.length() ? _otaErrorMsg : String(Update.errorString());
    r += F("\"}");
    webServer.send(500, F("application/json"), r);
    return;
  }
  webServer.send(200, F("application/json"), F("{\"ok\":true}"));
  _pendingRestart = true;
}

// ── Init ───────────────────────────────────────────────────────────────────────
inline void initWebServer(PedalState &pedal) {
  _webPedal = &pedal;

  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/api/state", HTTP_GET, handleGetState);
  webServer.on("/api/presets", HTTP_GET, handleGetPresets);
  webServer.on("/api/presets", HTTP_OPTIONS, handleOPTIONS);
  webServer.on("/api/channel", HTTP_POST, handlePostChannel);
  webServer.on("/api/channel", HTTP_OPTIONS, handleOPTIONS);
  webServer.on("/api/bpm", HTTP_POST, handlePostBpm);
  webServer.on("/api/bpm", HTTP_OPTIONS, handleOPTIONS);
  webServer.on("/api/poll", HTTP_GET, handleGetPoll);
  webServer.on("/api/poll", HTTP_OPTIONS, handleOPTIONS);
  webServer.on("/api/pot", HTTP_POST, handlePostPot);
  webServer.on("/api/pot", HTTP_OPTIONS, handleOPTIONS);
  webServer.on("/api/global", HTTP_GET, handleGetGlobal);
  webServer.on("/api/global", HTTP_POST, handlePostGlobal);
  webServer.on("/api/global", HTTP_OPTIONS, handleOPTIONS);
  webServer.on("/api/expcal", HTTP_GET, handleGetExpCal);
  webServer.on("/api/expcal", HTTP_POST, handlePostExpCal);
  webServer.on("/api/expcal", HTTP_OPTIONS, handleOPTIONS);
  webServer.on("/api/backup", HTTP_GET, handleGetBackup);
  webServer.on("/api/backup", HTTP_OPTIONS, handleOPTIONS);
  webServer.on("/api/restore", HTTP_POST, handlePostRestore);
  webServer.on("/api/restore", HTTP_OPTIONS, handleOPTIONS);
  webServer.on("/api/factory-reset", HTTP_POST, handlePostFactoryReset);
  webServer.on("/api/factory-reset", HTTP_OPTIONS, handleOPTIONS);
  webServer.on("/api/ota", HTTP_POST, handleOTAResponse, handleOTAUpload);
  webServer.on("/api/ota", HTTP_OPTIONS, handleOPTIONS);

  webServer.on("/api/multis", HTTP_GET, handleGetMultiScenes);
  webServer.on("/api/multis", HTTP_POST, handlePostMultiScenes);
  webServer.on("/api/multis", HTTP_OPTIONS, handleOPTIONS);
  for(int i = 0; i < (int)MAX_MULTI_SCENES; i++) {
    char path[24];
    snprintf(path, sizeof(path), "/api/multis/%d", i);
    webServer.on(path, HTTP_DELETE, [i]() { handleDeleteMultiScene(i); });
    webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
  }

  webServer.on("/dismiss", HTTP_GET, handleDismiss);

  // Captive-portal catch-all — any unknown path redirects to the UI root.
  webServer.onNotFound(handleCaptivePortal);

  for(int i = 0; i < 6; i++) {
    char path[26];
    snprintf(path, sizeof(path), "/api/button/%d", i);
    webServer.on(path, HTTP_POST, [i]() { handlePostButton(i); });
    webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
    snprintf(path, sizeof(path), "/api/button/%d/press", i);
    webServer.on(path, HTTP_POST, [i]() { handleButtonPress(i); });
    webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
    snprintf(path, sizeof(path), "/api/button/%d/release", i);
    webServer.on(path, HTTP_POST, [i]() { handleButtonRelease(i); });
    webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
    snprintf(path, sizeof(path), "/api/button/%d/action", i);
    webServer.on(path, HTTP_POST, [i]() { handlePostButtonAction(i); });
    webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
  }

  for(int i = 0; i < NUM_PRESETS; i++) {
    char path[26];
    snprintf(path, sizeof(path), "/api/preset/load/%d", i);
    webServer.on(path, HTTP_POST, [i]() { handlePresetLoad(i); });
    webServer.on(path, HTTP_OPTIONS, handleOPTIONS);
    snprintf(path, sizeof(path), "/api/preset/save/%d", i);
    webServer.on(path, HTTP_POST, [i]() { handlePresetSave(i); });
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
    if(_dnsRunning)
      dnsServer.processNextRequest();
    webServer.handleClient();
  }

  // Restart after OTA response has been sent — delay lets TCP flush first.
  if(_pendingRestart) {
    delay(500);
    ESP.restart();
  }
}
