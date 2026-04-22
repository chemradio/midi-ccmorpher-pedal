#pragma once
#include "midiClock.h"
#include "../pedalState.h"
#include "../config.h"

inline void syncClockRamps(PedalState &pedal) {
  if(pedal.globalSettings.perFsModulator) {
    for(int i = 0; i < (int)pedal.buttons.size(); i++) {
      FSButton &ab = pedal.buttons[i];
      if(ab.isModSwitch) {
        if(ab.rampUpMs   & CLOCK_SYNC_FLAG)
          pedal.modulators[i].rampUpTimeMs   = midiClock.syncToMs(ab.rampUpMs);
        if(ab.rampDownMs & CLOCK_SYNC_FLAG)
          pedal.modulators[i].rampDownTimeMs = midiClock.syncToMs(ab.rampDownMs);
      }
    }
  } else if(pedal.lastActiveFSIndex >= 0) {
    FSButton &ab = pedal.buttons[pedal.lastActiveFSIndex];
    if(ab.isModSwitch) {
      if(ab.rampUpMs   & CLOCK_SYNC_FLAG)
        pedal.modulators[0].rampUpTimeMs   = midiClock.syncToMs(ab.rampUpMs);
      if(ab.rampDownMs & CLOCK_SYNC_FLAG)
        pedal.modulators[0].rampDownTimeMs = midiClock.syncToMs(ab.rampDownMs);
    }
  }
}
