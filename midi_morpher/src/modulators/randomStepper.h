#pragma once
#include <stdlib.h>

inline void MidiCCModulator::updateRandomStepper() {
  // On deactivation: snap to rest position and stop.
  if(!isActivated) {
    uint16_t rest = restVal();
    if(currentValue != rest) {
      currentValue = rest;
      emit(true);
    }
    isModulating = false;
    return;
  }

  unsigned long now      = millis();
  unsigned long interval = (currentValue <= minValue14) ? rampUpTimeMs
                         : (currentValue >= maxValue14) ? rampDownTimeMs
                         : rampUpTimeMs;

  if(now - lastRandomTime < interval) return;
  lastRandomTime = now;

  uint16_t range = (maxValue14 > minValue14) ? (uint16_t)(maxValue14 - minValue14) : 0;
  if(range == 0) {
    currentValue = minValue14;
    emit(true);
    return;
  }

  // Pick a new random value, avoiding immediate repetition (up to 8 tries).
  uint16_t newVal   = (uint16_t)(minValue14 + (rand() % ((uint32_t)range + 1)));
  uint8_t  attempts = 0;
  while(newVal == currentValue && attempts < 8) {
    newVal = (uint16_t)(minValue14 + (rand() % ((uint32_t)range + 1)));
    attempts++;
  }

  currentValue = newVal;
  emit(true);
}
