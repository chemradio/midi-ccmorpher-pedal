#pragma once
#include <Arduino.h>

enum ModulationType {
  NOMODULATION,
  RAMPER,
  LFO,
  STEPPER,
  RANDOM
};

// On-disk twin of FSAction. Kept here (not in statePersistence.h) so that
// pedalState.h can carry a live copy without a cyclic include.
struct FSActionPersisted {
    uint8_t  enabled;
    uint8_t  modeIndex;
    uint8_t  midiNumber;
    uint8_t  fsChannel;
    uint8_t  ccLow;
    uint8_t  ccHigh;
    uint8_t  velocity;
    uint8_t  _pad;
    uint32_t rampUpMs;
    uint32_t rampDownMs;
};  // 16 bytes
