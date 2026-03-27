#pragma once
#include <math.h>

inline void MidiCCModulator::updateLFO() {
  uint8_t distance = abs(targetValue - currentValue);
  if(distance == 0) {
    invert(); // reverse direction
    return;
  }

  // scale ramp time based on already travelled distance
  float fraction = distance / 127.0f; // get the fraction based on travelled distance.
  bool goingUp = (targetValue > currentValue);
  unsigned long fullDuration = goingUp ? rampUpTimeMs : rampDownTimeMs;
  currentRampDuration = fullDuration * fraction;

  if(currentRampDuration <= 1) {
    currentValue = targetValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
    invert(); // reverse direction
    return;
  }

  unsigned long now = millis();
  unsigned long elapsed = now - rampStartTime;

  //   early exit if overtime ramping
  if(elapsed >= currentRampDuration) {
    currentValue = targetValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
    invert(); // reverse direction
    return;
  }

  float t = (float)elapsed / currentRampDuration;
  if(t > 1.0f)
    t = 1.0f;

  float shaped = shapeRamp(t, rampShape);

  int delta = targetValue - rampStartValue;
  uint8_t newValue = rampStartValue + (delta * shaped);

  if(newValue != currentValue) {
    currentValue = newValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
  }
}
