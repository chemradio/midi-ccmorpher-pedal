#pragma once
#include "../midiOut.h"

inline const uint8_t potDeadband = 20;
uint16_t readPotMedian(uint8_t pin) {
  uint16_t a = analogRead(pin);
  uint16_t b = analogRead(pin);
  uint16_t c = analogRead(pin);
  uint16_t minv = min(a, min(b, c));
  uint16_t maxv = max(a, max(b, c));
  return a + b + c - minv - maxv; // middle value
}

uint16_t readPotAvg(uint8_t pin) {
  uint32_t sum = 0;
  for(int i = 0; i < 4; i++) {
    sum += analogRead(pin);
  }
  return sum >> 2; // divide by 4
}

struct AnalogPot {
  uint8_t pin;
  const char *name;
  uint8_t midiCCNumber;
  uint16_t lastValue = 0;
  uint8_t lastMidiValue = 0;
  bool ccDisplayDirty = false;
  long ccLastDisplayDirty = 0;

  void sendMidiCC(uint8_t midiChannel) {
    sendMIDI(midiChannel, false, midiCCNumber, lastMidiValue);
  }
};
