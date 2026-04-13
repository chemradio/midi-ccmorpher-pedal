#pragma once
#include <USB.h>
#include <USBMIDI.h>

extern USBMIDI midi;

// TinyUSB API — returns true only when a USB host has enumerated the device.
// Used to skip USB MIDI writes when no host is connected (writes block otherwise).
extern "C" bool tud_mounted(void);

inline void sendMIDI(uint8_t channel, bool isPC, uint8_t number, uint8_t value = 0) {
  if(isPC) {
    Serial1.write(0xC0 | channel);
    Serial1.write(number);
    midi.programChange(number, channel + 1);
  } else {
    Serial1.write(0xB0 | channel);
    Serial1.write(number);
    Serial1.write(value);
    midi.controlChange(number, value, channel + 1);
  }
}

// Send a MIDI Timing Clock byte (0xF8) to DIN + USB.
// USB write is guarded by tud_mounted(): without a host the send buffer fills
// up and write() blocks, triggering the watchdog.
inline void sendClockTick() {
  Serial1.write(0xF8);
  if(tud_mounted()) midi.write(0xF8);
}

inline void sendNote(uint8_t channel, uint8_t note, bool on, uint8_t velocity = 100) {
  Serial1.write((on ? 0x90 : 0x80) | channel);
  Serial1.write(note & 0x7F);
  Serial1.write(on ? (velocity & 0x7F) : 0);
  if(on) {
    midi.noteOn(note, velocity, channel + 1);
  } else {
    midi.noteOff(note, 0, channel + 1);
  }
}
