#pragma once

// Maps a continuous 0–127 value to the nearest step boundary.
inline uint8_t quantize(uint8_t value, uint8_t steps) {
  if (steps <= 1) return 0;
  uint16_t stepSize = 127 / (steps - 1);
  uint8_t  index    = (value + stepSize / 2) / stepSize;
  return (uint8_t)(index * stepSize);
}

inline void MidiCCModulator::updateStepper() {
  if (currentValue == targetValue) { isModulating = false; return; }

  uint8_t newValue;
  bool done = calcRampValue(newValue);
  if (done) isModulating = false;

  newValue = quantize(newValue, stepperSteps);

  if (newValue != currentValue) {
    currentValue = newValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
  }
}
