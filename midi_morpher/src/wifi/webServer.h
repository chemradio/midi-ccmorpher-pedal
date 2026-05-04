#pragma once
#include "../clock/midiClock.h"
#include "../controls/hidKeyboard.h"
#include "../expression/expInput.h"
#include "../footswitches/footswitch.h"
#include "../pedalState.h"
#include "../statePersistence.h"
#include "midiUdpBroadcast.h"
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
  if(_webPedal && _webPedal->globalSettings.udpEnabled) udpBroadcastInit();
  _apRunning = true;
}

inline void stopAP() {
  udpBroadcastDeinit();
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

#include "webJson.h"
#include "webApi.h"

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
