#pragma once
#include <USB.h>
#include <USBMIDI.h>
#include "ble/bleMidi.h"

extern USBMIDI midi;

// TinyUSB API — returns true only when a USB host has enumerated the device.
// Used to skip USB MIDI writes when no host is connected (writes block otherwise).
extern "C" bool tud_mounted(void);

inline void sendMIDI(uint8_t channel, bool isPC, uint8_t number, uint8_t value = 0) {
  if(isPC) {
    Serial1.write(0xC0 | channel);
    Serial1.write(number);
    midi.programChange(number, channel + 1);
    uint8_t b[2] = { (uint8_t)(0xC0 | channel), number };
    bleMidiSendBytes(b, 2);
  } else {
    Serial1.write(0xB0 | channel);
    Serial1.write(number);
    Serial1.write(value);
    midi.controlChange(number, value, channel + 1);
    uint8_t b[3] = { (uint8_t)(0xB0 | channel), number, value };
    bleMidiSendBytes(b, 3);
  }
}

// Send a MIDI Timing Clock byte (0xF8) to DIN + USB + BLE.
// USB write is guarded by tud_mounted(): without a host the send buffer fills
// up and write() blocks, triggering the watchdog.
inline void sendClockTick() {
  Serial1.write(0xF8);
  if(tud_mounted()) midi.write(0xF8);
  uint8_t b[1] = { 0xF8 };
  bleMidiSendBytes(b, 1);
}

inline void sendNote(uint8_t channel, uint8_t note, bool on, uint8_t velocity = 100) {
  uint8_t status = (on ? 0x90 : 0x80) | channel;
  uint8_t vel    = on ? (velocity & 0x7F) : 0;
  Serial1.write(status);
  Serial1.write(note & 0x7F);
  Serial1.write(vel);
  if(on) {
    midi.noteOn(note, velocity, channel + 1);
  } else {
    midi.noteOff(note, 0, channel + 1);
  }
  uint8_t b[3] = { status, (uint8_t)(note & 0x7F), vel };
  bleMidiSendBytes(b, 3);
}

// ── System / Transport helpers ───────────────────────────────────────────────

// Send a single-byte System Real-Time message (0xF8–0xFF). DIN + USB + BLE.
inline void sendSystemRealtime(uint8_t byte) {
  Serial1.write(byte);
  if(tud_mounted()) midi.write(byte);
  uint8_t b[1] = { byte };
  bleMidiSendBytes(b, 1);
}

// Send an MMC (MIDI Machine Control) SysEx command:
//   F0 7F 7F 06 <cmd> F7
// Standard broadcast device ID 0x7F. DIN bytes; USB as two midi packets
// (CIN 0x4 SysEx start/continue + CIN 0x7 SysEx end with 3 bytes); BLE as a
// single notify wrapped with BLE-MIDI timestamps around F0 and F7.
inline void sendMMC(uint8_t cmd) {
  const uint8_t bytes[6] = { 0xF0, 0x7F, 0x7F, 0x06, cmd, 0xF7 };
  for(uint8_t i = 0; i < 6; i++) Serial1.write(bytes[i]);
  if(tud_mounted()) {
    midiEventPacket_t p1 = { 0x04, bytes[0], bytes[1], bytes[2] };
    midiEventPacket_t p2 = { 0x07, bytes[3], bytes[4], bytes[5] };
    midi.writePacket(&p1);
    midi.writePacket(&p2);
  }
  bleMidiSendSysEx(bytes, 6);
}

// Send a Song Position Pointer (F2 lsb msb). 14-bit position in MIDI beats.
// DIN + USB (USB as CIN 0x3 system-common 3-byte packet) + BLE.
inline void sendSPP(uint16_t pos) {
  uint8_t lsb = pos & 0x7F;
  uint8_t msb = (pos >> 7) & 0x7F;
  Serial1.write(0xF2);
  Serial1.write(lsb);
  Serial1.write(msb);
  if(tud_mounted()) {
    midiEventPacket_t p = { 0x03, 0xF2, lsb, msb };
    midi.writePacket(&p);
  }
  uint8_t b[3] = { 0xF2, lsb, msb };
  bleMidiSendBytes(b, 3);
}
