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
  unsigned long interval = (currentValue <= randomMin) ? rampUpTimeMs
                         : (currentValue >= randomMax) ? rampDownTimeMs
                         : rampUpTimeMs;

  if(now - lastRandomTime < interval) return;
  lastRandomTime = now;

  uint16_t range = (randomMax > randomMin) ? (uint16_t)(randomMax - randomMin) : 0;
  if(range == 0) {
    currentValue = randomMin;
    emit(true);
    return;
  }

  // Pick a new random value, avoiding immediate repetition (up to 8 tries).
  uint16_t newVal   = (uint16_t)(randomMin + (rand() % ((uint32_t)range + 1)));
  uint8_t  attempts = 0;
  while(newVal == currentValue && attempts < 8) {
    newVal = (uint16_t)(randomMin + (rand() % ((uint32_t)range + 1)));
    attempts++;
  }

  currentValue = newVal;
  emit(true);
}
