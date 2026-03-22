#pragma once

enum ModulationType {
  NOMODULATION,
  RAMPER,
  LFO,
  STEPPER,
  RANDOM
};

enum class PotMode {
  RampSpeed,
  SendCC
};