#pragma once
#include <math.h>

inline void MidiCCModulator::updateLFO() {
  // Reached the segment target — either bounce or finish.
  if (currentValue == targetValue) {
    if (lfoFinishing) {
      isModulating  = false;
      lfoFinishing  = false;
      lfoTowardRest = false;
      sendMIDI(midiChannel, false, midiCCNumber, currentValue);
      return;
    }
    // Bounce: flip direction and queue the next segment.
    lfoTowardRest  = !lfoTowardRest;
    targetValue    = lfoTowardRest ? restVal() : activeVal();
    rampStartValue = currentValue;
    rampStartTime  = millis();
    return;
  }

  uint8_t newValue;
  if (calcRampValue(newValue)) {
    // Segment complete mid-ramp — snap and send; bounce handled next iteration.
    currentValue = targetValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
    return;
  }

  if (newValue != currentValue) {
    currentValue = newValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
  }
}
