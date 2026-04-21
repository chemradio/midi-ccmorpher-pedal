#pragma once

inline void MidiCCModulator::updateRamper() {
  if (currentValue == targetValue) { isModulating = false; return; }

  uint16_t newValue;
  bool done = calcRampValue(newValue);
  if (done) isModulating = false;

  if (newValue != currentValue) {
    currentValue = newValue;
    emit(done);
  }
}
