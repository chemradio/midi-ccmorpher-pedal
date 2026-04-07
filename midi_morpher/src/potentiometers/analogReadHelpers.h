#pragma once
#include "../midiOut.h"

// ── Noise filtering constants ─────────────────────────────────────────────────
// Deadband on the EMA-filtered value. 20 / 4096 ≈ 0.5% of full range.
inline constexpr uint16_t POT_DEADBAND = 20;

// ── Raw read helper ───────────────────────────────────────────────────────────
// 8-sample average; reduces ESP32 ADC noise before EMA stage.
inline uint16_t readPotAvg(uint8_t pin) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < 8; i++) sum += analogRead(pin);
  return (uint16_t)(sum >> 3);
}

// ── AnalogPot ─────────────────────────────────────────────────────────────────

struct AnalogPot {
  uint8_t    pin;
  const char *name;
  uint8_t    midiCCNumber;

  uint16_t filtered     = 0;     // EMA accumulator
  uint16_t lastValue    = 0;     // last value that crossed the deadband
  uint8_t  lastMidiValue = 0;
  bool     initialized  = false; // seed EMA on first read
  bool     ccDisplayDirty     = false;
  long     ccLastDisplayDirty = 0;

  // Call every loop iteration. Returns true only when the filtered value has
  // moved far enough from lastValue to be considered intentional. On true,
  // lastValue has already been updated to the new position.
  bool update() {
    uint16_t raw = readPotAvg(pin);

    if(!initialized) {
      filtered    = raw;
      lastValue   = raw;
      initialized = true;
      return false;
    }

    // IIR low-pass filter, alpha = 1/4 (bitshift, no floats in hot path).
    // New filtered = 0.75 × old + 0.25 × raw
    filtered = filtered - (filtered >> 2) + (raw >> 2);

    if((uint16_t)abs((int)filtered - (int)lastValue) > POT_DEADBAND) {
      lastValue = filtered;
      return true;
    }
    return false;
  }

  void sendMidiCC(uint8_t midiChannel) {
    sendMIDI(midiChannel, false, midiCCNumber, lastMidiValue);
  }
};
