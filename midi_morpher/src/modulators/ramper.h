#pragma once

inline void MidiCCModulator::updateRamper() {
  uint8_t distance = abs(targetValue - currentValue);
  if(distance == 0) {
    isModulating = false;
    return;
  }

  // scale ramp time based on already travelled distance
  float fraction = distance / 127.0f; // get the fraction based on travelled distance.
  bool goingUp = (targetValue > currentValue);
  unsigned long fullDuration = goingUp ? rampUpTimeMs : rampDownTimeMs;
  currentRampDuration = fullDuration * fraction;

  //   early exit if already close
  if(currentRampDuration <= 1) {
    currentValue = targetValue;
    isModulating = false;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
    return;
  }

  unsigned long now = millis();
  unsigned long elapsed = now - rampStartTime;

  //   early exit if overtime ramping
  if(elapsed >= currentRampDuration) {
    currentValue = targetValue;
    isModulating = false;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
    return;
  }

  float t = (float)elapsed / currentRampDuration;
  if(t > 1.0f)
    t = 1.0f;

  float shaped;

  if(!isLinear) {
    shaped = t * t;
    // float inv = 1.0f - t;
    // shaped = 1.0f - (inv * inv * inv);
  } else // linear
  {
    shaped = t;
  }

  int delta = targetValue - rampStartValue;
  uint8_t newValue = rampStartValue + (delta * shaped);

  if(newValue != currentValue) {
    currentValue = newValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
  }
}