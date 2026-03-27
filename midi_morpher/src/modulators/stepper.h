#pragma once

inline uint8_t quantize(uint8_t value, uint8_t steps) {
  if(steps <= 1)
    return 0;

  uint16_t stepSize = 127 / (steps - 1);
  uint8_t index = (value + stepSize / 2) / stepSize;
  return index * stepSize;
}

inline uint8_t quantize_rounded(uint8_t value, uint8_t steps) {
  if(steps <= 1)
    return 0;
  uint8_t index = ((uint16_t)value * (uint16_t)(steps - 1) + 63) / 127;
  return ((uint16_t)index * 127 + (steps / 2)) / (steps - 1);
}

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

  float shaped = shapeRamp(t, rampShape);

  int delta = targetValue - rampStartValue;
  uint8_t newValue = rampStartValue + (delta * shaped);
  newValue = quantize(newValue, stepperSteps);

  if(newValue != currentValue) {
    currentValue = newValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
  }
}
