#pragma once
#include <USB.h>
#include <USBMIDI.h>

extern USBMIDI midi;

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
