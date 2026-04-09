#pragma once

inline void MidiCCModulator::updateRamper() {
  if (currentValue == targetValue) { isModulating = false; return; }

  uint8_t newValue;
  bool done = calcRampValue(newValue);
  if (done) isModulating = false;

  if (newValue != currentValue) {
    currentValue = newValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
  }
}
