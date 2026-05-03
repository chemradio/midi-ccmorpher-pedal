#pragma once
#include <Arduino.h>
#include <USB.h>
#include <USBMIDI.h>
#include "../config.h"

extern USBMIDI usbMidi;
extern "C" bool tud_mounted(void);

inline void midiOutInit() {
  Serial1.begin(31250, SERIAL_8N1, -1, MIDI_TX_PIN);
}

// Feed raw MIDI bytes. Accumulates into complete messages for USB MIDI packets;
// every byte is also forwarded immediately to DIN UART.
inline void midiOutBytes(const uint8_t *data, size_t len) {
  static uint8_t runStatus = 0;
  static uint8_t msgBuf[3] = {};
  static uint8_t msgLen    = 0;
  static uint8_t msgNeed   = 0;

  for (size_t i = 0; i < len; i++) {
    uint8_t b = data[i];
    Serial1.write(b);

    // System realtime — single byte, send immediately, don't disturb msg state.
    if (b >= 0xF8) {
      if (tud_mounted()) {
        midiEventPacket_t p = {0x0F, b, 0, 0};
        usbMidi.writePacket(&p);
      }
      continue;
    }

    if (b & 0x80) {
      // Status byte
      runStatus = (b < 0xF0) ? b : 0;
      msgBuf[0] = b;
      msgLen    = 1;
      uint8_t hi = b & 0xF0;
      if (b >= 0xF0) {
        // System common: F1=2, F2=3, F3=2, F6=1, F7=1 — handle F0/F7 sysex elsewhere
        msgNeed = (b == 0xF2) ? 3 : (b == 0xF1 || b == 0xF3) ? 2 : 1;
      } else {
        msgNeed = (hi == 0xC0 || hi == 0xD0) ? 2 : 3;
      }
    } else {
      // Data byte — use running status if no current message started
      if (msgLen == 0 && runStatus) {
        msgBuf[0] = runStatus;
        msgLen    = 1;
        uint8_t hi = runStatus & 0xF0;
        msgNeed   = (hi == 0xC0 || hi == 0xD0) ? 2 : 3;
      }
      if (msgLen > 0 && msgLen < 3)
        msgBuf[msgLen++] = b;
    }

    if (msgLen > 0 && msgLen >= msgNeed) {
      if (tud_mounted()) {
        // CIN: channel-voice = high nibble of status; system-common = packet length code
        // (1B → 5, 2B → 2, 3B → 3). USB MIDI 1.0 spec.
        uint8_t cin;
        if (msgBuf[0] >= 0xF0) {
          cin = (msgLen == 1) ? 0x5 : (msgLen == 2) ? 0x2 : 0x3;
        } else {
          cin = msgBuf[0] >> 4;
        }
        midiEventPacket_t p = {
          cin,
          msgBuf[0],
          msgLen > 1 ? msgBuf[1] : 0,
          msgLen > 2 ? msgBuf[2] : 0
        };
        usbMidi.writePacket(&p);
      }
      msgLen = 0;
    }
  }
}
