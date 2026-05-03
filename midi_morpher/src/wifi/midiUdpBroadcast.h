#pragma once
#include <WiFiUdp.h>

#define UDP_MIDI_PORT 5005

static WiFiUDP _udpMidi;
static bool    _udpMidiActive = false;

inline void udpBroadcastInit() {
  _udpMidi.begin(UDP_MIDI_PORT);
  _udpMidiActive = true;
}

inline void udpBroadcastDeinit() {
  _udpMidi.stop();
  _udpMidiActive = false;
}

inline bool udpBroadcastActive() { return _udpMidiActive; }

inline void udpBroadcastSend(const uint8_t *bytes, size_t len) {
  if (!_udpMidiActive || len == 0) return;
  // 192.168.4.255 = directed broadcast on the Morpher's AP subnet
  _udpMidi.beginPacket(IPAddress(192, 168, 4, 255), UDP_MIDI_PORT);
  _udpMidi.write(bytes, len);
  _udpMidi.endPacket();
}
