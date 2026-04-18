#pragma once
#include "../config.h"
#include "../midiOut.h"
#include "../pedalState.h"
#include "../potentiometers/analogReadHelpers.h"
#include "../visual/display.h"

inline AnalogPot expInput = {EXP_IN, "Exp", 20, 20};

inline void initExpInput() {
  pinMode(EXP_IN, INPUT);
}

inline void handleExpInput(PedalState &pedal) {
  static uint16_t ema    = 0;
  static uint16_t prev   = 0;
  static uint16_t expMin = 4095, expMax = 0;
  static bool     init   = false;

  uint16_t raw = readPotAvg(EXP_IN);
  if (!init) { ema = prev = raw; init = true; return; }

  ema = (ema + raw) >> 1;                          // alpha = 0.5
  if ((uint16_t)abs((int)ema - (int)prev) <= 20) return;
  prev = ema;

  if (ema < expMin) expMin = ema;
  if (ema > expMax) expMax = ema;
  uint16_t span    = expMax - expMin;
  uint16_t effSpan = span > 20 ? span - 20 : span; // top dead-zone → reliable 127
  uint8_t midiVal  = (span < 50) ? 0
      : (uint8_t)constrain((int32_t)(ema - expMin) * 128 / effSpan, 0, 127);

  if (midiVal == expInput.lastMidiValue) return;
  expInput.lastMidiValue = midiVal;
  sendMIDI(pedal.midiChannel, false, pedal.globalSettings.expCC, midiVal);
  if (pedal.globalSettings.expWakesDisplay)
    displayExpInput(pedal.globalSettings.expCC, midiVal);
}
