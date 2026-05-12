#pragma once

// Maps a continuous value to the nearest step boundary within [minV, maxV].
inline uint16_t quantize(uint16_t value, uint8_t steps, uint16_t minV, uint16_t maxV) {
  if (steps <= 1 || maxV <= minV) return minV;
  uint32_t range    = (uint32_t)(maxV - minV);
  uint32_t stepSize = range / (uint32_t)(steps - 1);
  if (stepSize == 0) return minV;
  if (value <= minV) return minV;
  uint32_t off   = (uint32_t)value - (uint32_t)minV;
  uint32_t index = (off + stepSize / 2) / stepSize;
  if (index > (uint32_t)(steps - 1)) index = (uint32_t)(steps - 1);
  return (uint16_t)((uint32_t)minV + index * stepSize);
}

inline void MidiCCModulator::updateStepper() {
  if (currentValue == targetValue) { isModulating = false; return; }

  uint16_t newValue;
  bool done = calcRampValue(newValue);
  if (done) isModulating = false;

  newValue = quantize(newValue, stepperSteps, minValue14, maxValue14);

  if (newValue != currentValue) {
    currentValue = newValue;
    emit(done);
  }
}
