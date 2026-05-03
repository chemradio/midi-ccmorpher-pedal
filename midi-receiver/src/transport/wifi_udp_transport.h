#pragma once
#include <WiFi.h>
#include <WiFiUdp.h>
#include "transport.h"
#include "../../config.h"

class WifiUdpTransport : public Transport {
public:
  void begin(MidiDataCallback cb) override {
    _cb = cb;
    _connected = false;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    _connectStart = millis();
    Serial.println(F("[UDP] Connecting to MIDI Morpher AP..."));
  }

  void loop() override {
    // (Re)connect if needed
    if (WiFi.status() != WL_CONNECTED) {
      if (_connected) {
        _connected = false;
        Serial.println(F("[UDP] WiFi lost, reconnecting..."));
      }
      if (millis() - _connectStart > 10000) {
        WiFi.reconnect();
        _connectStart = millis();
      }
      return;
    }

    if (!_connected) {
      _connected = true;
      _udp.begin(UDP_MIDI_PORT);
      Serial.print(F("[UDP] Connected. IP: "));
      Serial.println(WiFi.localIP());
    }

    // Drain all queued packets so a burst (e.g. clock + note-on) doesn't smear
    // across loop iterations.
    uint8_t buf[256];
    while (_udp.parsePacket() > 0) {
      int len = _udp.read(buf, sizeof(buf));
      if (len > 0 && _cb) _cb(buf, (size_t)len);
    }
  }

  void stop() override {
    _udp.stop();
    WiFi.disconnect(true);
    _connected = false;
  }

  bool connected() const override { return _connected; }

private:
  MidiDataCallback _cb          = nullptr;
  WiFiUDP          _udp;
  bool             _connected   = false;
  unsigned long    _connectStart = 0;
};
