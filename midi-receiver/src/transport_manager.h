#pragma once
#include <Preferences.h>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "transport/transport.h"
#include "transport/wifi_udp_transport.h"
#include "transport/espnow_transport.h"
#include "../config.h"

struct _TEntry {
  Transport*  ptr;
  const char* name;
  uint32_t    color;   // packed 0x00RRGGBB at full brightness; scaled by setBrightness()
};

static WifiUdpTransport _tUdp;
static EspNowTransport  _tEspNow;

static _TEntry _tList[] = {
  { &_tUdp,    "WiFi UDP", 0xFF8000 },  // orange
  { &_tEspNow, "ESP-NOW",  0xA020F0 },  // purple
};
static const uint8_t _tCount = sizeof(_tList) / sizeof(_tList[0]);

#if STATUS_NEOPIXEL_PIN >= 0
static Adafruit_NeoPixel _statusPx(1, STATUS_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
#endif

class TransportManager {
public:
  void begin(MidiDataCallback cb) {
    _cb = cb;

    Preferences prefs;
    prefs.begin("rxcfg", true);
    _idx = prefs.getUChar("transport", 0);
    prefs.end();
    if (_idx >= _tCount) _idx = 0;

#if STATUS_NEOPIXEL_PIN >= 0
    _statusPx.begin();
    _statusPx.setBrightness(STATUS_NEOPIXEL_BRIGHTNESS);
    _statusPx.clear();
    _statusPx.show();
#endif
    pinMode(TRANSPORT_SELECT_BTN_PIN, INPUT_PULLUP);

    _startCurrent();
  }

  void loop() {
    _handleButton();
    _tList[_idx].ptr->loop();
    _updateStatusLed();
  }

private:
  MidiDataCallback _cb       = nullptr;
  uint8_t          _idx      = 0;
  bool             _btnLast  = true;   // HIGH = not pressed (pull-up)
  unsigned long    _btnTime  = 0;
  unsigned long    _blinkTime = 0;
  bool             _blinkOn   = false;
  bool             _wasConnected = false;

  void _startCurrent() {
    Serial.printf("[RX] Transport: %s\n", _tList[_idx].name);
    _tList[_idx].ptr->begin(_cb);
    _wasConnected = false;
    _blinkOn      = false;
    _blinkTime    = millis();
    _setStatusColor(0);   // start dark; loop will blink/solid as appropriate
  }

  // Blink the status pixel while the active transport is connecting; switch
  // to solid as soon as connected() returns true.
  void _updateStatusLed() {
    bool connected = _tList[_idx].ptr->connected();
    if (connected) {
      if (!_wasConnected) {
        _setStatusColor(_tList[_idx].color);
        _wasConnected = true;
      }
      return;
    }
    if (_wasConnected) {
      _wasConnected = false;
      _blinkOn      = false;
      _blinkTime    = millis();
    }
    unsigned long now = millis();
    if (now - _blinkTime >= 500) {
      _blinkTime = now;
      _blinkOn   = !_blinkOn;
      _setStatusColor(_blinkOn ? _tList[_idx].color : 0);
    }
  }

  void _handleButton() {
    bool btn = digitalRead(TRANSPORT_SELECT_BTN_PIN);
    if (_btnLast && !btn) {                     // HIGH→LOW falling edge
      unsigned long now = millis();
      if (now - _btnTime > 50) {                // 50 ms debounce
        _btnTime = now;
        _tList[_idx].ptr->stop();
        _idx = (_idx + 1) % _tCount;
        Preferences prefs;
        prefs.begin("rxcfg", false);
        prefs.putUChar("transport", _idx);
        prefs.end();
        _startCurrent();
      }
    }
    _btnLast = btn;
  }

  void _setStatusColor(uint32_t rgb) {
#if STATUS_NEOPIXEL_PIN >= 0
    _statusPx.setPixelColor(0, rgb);
    _statusPx.show();
#endif
  }
};
