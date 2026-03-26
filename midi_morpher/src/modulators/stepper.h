#pragma once

// Stepper additional state — add these to MidiCCModulator if not present:
//   uint8_t stepSize = 10;            // CC values per step (1-127). Timing is automatic.
//   unsigned long lastStepTime = 0;   // timestamp of last step
//   bool stepperRampingDown = false;  // true when ramping back to 0 on release

// Timing model:
//   stepIntervalMs = rampUpTimeMs / (127 / stepSize)
//   This makes the staircase take the same total time as the smooth ramper.

inline void MidiCCModulator::updateStepper() {
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

  //   early exit if overtime ramping
  unsigned long now = millis();
  unsigned long elapsed = now - rampStartTime;
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
