#pragma once
#include <math.h>

inline void MidiCCModulator::updateLFO() {
  unsigned long now = millis();
  float elapsedSec = (now - lfoStartTime) / 1000.0f;
  float periodSec = 1.0f / lfoRateHz;

  // Normalised position within one full period: 0.0 -> 1.0
  float fullPhase = fmodf(elapsedSec, periodSec) / periodSec;

  // --- Detect zero crossing when finishing (latch off / momentary release) ---
  // The wave returns to 0 at fullPhase == 0.0 (start of each new period).
  // We check if we've just wrapped — tolerance of one update cycle at ~1kHz.
  if(lfoFinishing) {
    float tolerance = (periodSec > 0.0f) ? (1.0f / (periodSec * 1000.0f)) : 0.05f;
    if(fullPhase < tolerance) {
      currentValue = 0;
      isModulating = false;
      lfoFinishing = false;
      sendMIDI(midiChannel, false, midiCCNumber, currentValue);
      return;
    }
  }

  // --- Compute wave shape ---
  float shaped;

  if(isLinear) {
    // Triangle wave: 0 -> 1 -> 0 over one full period
    shaped = (fullPhase < 0.5f)
                 ? (fullPhase * 2.0f)
                 : (2.0f - fullPhase * 2.0f);
  } else {
    // Sine wave remapped so both half-cycles go 0 -> 127 -> 0:
    // |sin(x)| gives exactly that shape — peaks at 127, zero-crosses at period boundaries
    float radians = 2.0f * M_PI * lfoRateHz * elapsedSec;
    shaped = fabsf(sinf(radians));
  }

  uint8_t newVal = (uint8_t)(shaped * 127.0f);

  if(newVal != currentValue) {
    currentValue = newVal;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
  }
}
