#pragma once
#include "../config.h"
#include "../midiOut.h"
#include "../pedalState.h"
#include "../potentiometers/analogReadHelpers.h"

inline AnalogPot expInput = {EXP_IN, "Exp", 20};

inline void initExpInput() {
  pinMode(EXP_IN, INPUT);
}

inline void handleExpInput(PedalState &pedal) {
  if (!expInput.update()) return;
  uint8_t midiVal = (uint8_t)((expInput.lastValue * 128UL) / 4096);
  if (midiVal == expInput.lastMidiValue) return;
  expInput.lastMidiValue = midiVal;
  sendMIDI(pedal.midiChannel, false, expInput.midiCCNumber, midiVal);
}
