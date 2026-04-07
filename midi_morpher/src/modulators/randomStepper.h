#pragma once
#include <stdlib.h>

inline void MidiCCModulator::updateRandomStepper() {
  // On deactivation: snap to rest position and stop.
  if(!isActivated) {
    uint8_t rest = restVal();
    if(currentValue != rest) {
      currentValue = rest;
      sendMIDI(midiChannel, false, midiCCNumber, currentValue);
    }
    isModulating = false;
    return;
  }

  unsigned long now      = millis();
  unsigned long interval = (currentValue <= randomMin) ? rampUpTimeMs
                         : (currentValue >= randomMax) ? rampDownTimeMs
                         : rampUpTimeMs;

  if(now - lastRandomTime < interval) return;
  lastRandomTime = now;

  uint8_t range = (randomMax > randomMin) ? (randomMax - randomMin) : 0;
  if(range == 0) {
    currentValue = randomMin;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
    return;
  }

  // Pick a new random value, avoiding immediate repetition (up to 8 tries).
  uint8_t newVal   = randomMin + (uint8_t)(rand() % (range + 1));
  uint8_t attempts = 0;
  while(newVal == currentValue && attempts < 8) {
    newVal = randomMin + (uint8_t)(rand() % (range + 1));
    attempts++;
  }

  currentValue = newVal;
  sendMIDI(midiChannel, false, midiCCNumber, currentValue);
}
