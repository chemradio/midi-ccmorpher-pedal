#pragma once
#include "../config.h"
#include "../potentiometers/analogReadHelpers.h"

// Expression pedal input — not yet wired into the main loop.
// When implemented: read EXP_IN, mirror to digipot output, send as CC20.

inline AnalogPot expInput = {EXP_IN, "Exp In", 20};

inline void initExpInput() {
  pinMode(EXP_IN, INPUT);
}
