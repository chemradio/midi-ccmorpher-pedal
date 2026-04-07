#pragma once
#include <math.h>

inline void MidiCCModulator::updateLFO() {
  uint8_t distance = (uint8_t)abs((int)targetValue - (int)currentValue);

  if(distance == 0) {
    if(lfoFinishing) {
      // Reached rest value — stop.
      isModulating  = false;
      lfoFinishing  = false;
      lfoTowardRest = false;
      sendMIDI(midiChannel, false, midiCCNumber, currentValue);
      return;
    }
    // Bounce: flip direction and set new target.
    lfoTowardRest  = !lfoTowardRest;
    targetValue    = lfoTowardRest ? restVal() : activeVal();
    rampStartValue = currentValue;
    rampStartTime  = millis();
    return;
  }

  // Proportional duration based on remaining distance.
  float fraction = distance / 127.0f;
  bool  goingUp  = (targetValue > currentValue);
  unsigned long fullDuration = goingUp ? rampUpTimeMs : rampDownTimeMs;
  currentRampDuration = (unsigned long)(fullDuration * fraction);

  if(currentRampDuration <= 1) {
    currentValue = targetValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
    return;
  }

  unsigned long elapsed = millis() - rampStartTime;
  if(elapsed >= currentRampDuration) {
    currentValue = targetValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
    return;
  }

  float t      = (float)elapsed / (float)currentRampDuration;
  float shaped = shapeRamp(t, rampShape);
  int   delta  = (int)targetValue - (int)rampStartValue;
  uint8_t newValue = (uint8_t)((int)rampStartValue + (int)((float)delta * shaped));

  if(newValue != currentValue) {
    currentValue = newValue;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
  }
}
