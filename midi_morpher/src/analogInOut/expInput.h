#pragma once
#include "../config.h"
#include "../midiOut.h"
#include "../pedalState.h"
#include "../potentiometers/analogReadHelpers.h"
#include "../visual/display.h"
#include "../clock/midiClock.h"
#include "../statePersistance.h"

#define EXP_CAL_DURATION_MS 5000

inline AnalogPot expInput = {EXP_IN, "Exp", 20, 20};

inline uint16_t expMin  = 4095, expMax  = 0;
inline uint16_t expPrev = 0;
inline bool     expInit = false;

inline bool          expCalibrating = false;
inline unsigned long expCalStart    = 0;

inline void loadExpCalibration(PedalState &pedal) {
  uint16_t mn = pedal.globalSettings.expCalMin;
  uint16_t mx = pedal.globalSettings.expCalMax;
  if (mx > mn + 50) { expMin = mn; expMax = mx; expInit = true; }
}

inline void startExpCalibration() {
  expMin = 4095; expMax = 0; expInit = false;
  expCalibrating = true; expCalStart = millis();
}

inline void initExpInput() {
  pinMode(EXP_IN, INPUT);
}

inline void handleExpInput(PedalState &pedal) {
  uint16_t raw = ((uint32_t)readPotAvg(EXP_IN) + readPotAvg(EXP_IN)) >> 1;

  if (expCalibrating) {
    unsigned long elapsed = millis() - expCalStart;
    if (elapsed >= EXP_CAL_DURATION_MS) {
      expCalibrating = false;
      pedal.globalSettings.expCalMin = expMin;
      pedal.globalSettings.expCalMax = expMax;
      saveGlobalSettings(pedal);
      displayExpCalibrating(0);
      return;
    }
    if (raw < expMin) expMin = raw;
    if (raw > expMax) expMax = raw;
    static uint8_t lastSecsLeft = 0xFF;
    uint8_t secsLeft = (uint8_t)((EXP_CAL_DURATION_MS - elapsed + 999) / 1000);
    if (secsLeft != lastSecsLeft) {
      lastSecsLeft = secsLeft;
      displayExpCalibrating(secsLeft);
    }
    return;
  }

  if (!expInit) { expPrev = raw; expInit = true; return; }

  if ((uint16_t)abs((int)raw - (int)expPrev) <= 8) return;
  expPrev = raw;

  if (raw < expMin) expMin = raw;
  if (raw > expMax) expMax = raw;
  uint16_t span    = expMax - expMin;
  uint16_t effSpan = span > 8 ? span - 8 : span;
  uint8_t midiVal  = (span < 50) ? 0
      : (uint8_t)constrain((int32_t)(raw - expMin) * 128 / effSpan, 0, 127);

  if (midiVal == expInput.lastMidiValue) return;
  expInput.lastMidiValue = midiVal;
  sendMIDI(pedal.midiChannel, false, pedal.globalSettings.expCC, midiVal);
  if (pedal.globalSettings.expWakesDisplay)
    displayExpInput(pedal.globalSettings.expCC, midiVal);
}
