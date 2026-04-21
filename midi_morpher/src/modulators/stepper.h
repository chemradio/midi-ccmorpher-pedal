#pragma once

// Maps a continuous 0–16383 value to the nearest step boundary.
inline uint16_t quantize(uint16_t value, uint8_t steps) {
  if (steps <= 1) return 0;
  uint32_t stepSize = (uint32_t)MOD_MAX_14BIT / (uint32_t)(steps - 1);
  uint32_t index    = ((uint32_t)value + stepSize / 2) / stepSize;
  return (uint16_t)(index * stepSize);
}

inline void MidiCCModulator::updateStepper() {
  if (currentValue == targetValue) { isModulating = false; return; }

  uint16_t newValue;
  bool done = calcRampValue(newValue);
  if (done) isModulating = false;

  newValue = quantize(newValue, stepperSteps);

  if (newValue != currentValue) {
    currentValue = newValue;
    emit(done);
  }
}
