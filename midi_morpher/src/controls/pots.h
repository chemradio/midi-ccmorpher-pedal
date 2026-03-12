#pragma once
#include "../config.h"
#include "../pedalState.h"
#include "../potentiometers/analogReadHelpers.h"
#include "../sharedTypes.h"

unsigned long lastSettingsBlockedDisplayed = 0;
unsigned long settingsBlockedDisplayTimeout = 1000;

inline AnalogPot analogPots[] = {{POT1_PIN, "UP Speed", POT1_CC},
                                 {POT2_PIN, "Down Speed", POT2_CC}};

inline void initAnalogPots() {
  for(int i = 0; i < 2; i++) {
    pinMode(analogPots[i].pin, INPUT);
    uint16_t medianRead = readPotMedian(analogPots[i].pin);
  }
}

inline void handleAnalogPot(
    AnalogPot &pot,
    PedalState &pedal,
    void (*displayCallback)(String, bool, uint8_t, long),
    void (*displayLockedMessage)(String)) {

  // uint16_t raw = analogRead(pot.pin);
  uint16_t selectedValue = readPotMedian(pot.pin);

  if(abs((int)selectedValue - (int)pot.lastValue) > potDeadband) {

    pot.lastValue = selectedValue;
    uint8_t midiScaled = (selectedValue * 128UL) / 4096;

    if(pedal.settingsLocked) {
      unsigned long now = millis();
      if((now - lastSettingsBlockedDisplayed) > settingsBlockedDisplayTimeout) {
        displayLockedMessage("pots");
        lastSettingsBlockedDisplayed = now;
      }
      return;
    }

    // clamp to ADC range
    if(selectedValue > 4095)
      selectedValue = 4095;

    if(pedal.potMode == PotMode::SendCC) {
      if(midiScaled != pot.lastMidiValue) {
        pot.lastMidiValue = midiScaled;
        pot.sendMidiCC(pedal.midiChannel);
        pot.ccDisplayDirty = true;
        pot.ccLastDisplayDirty = millis();

        displayCallback(
            "MidiCC: " + String(pot.midiCCNumber),
            true,
            midiScaled,
            0);
      }
    } else {
      // linear pot feel
      // long rampMs = map(selectedValue, 0, 4095, pedal.rampMinSpeedMs,
      // pedal.rampMaxSpeedMs);

      // exponential pot feel
      float normalized = selectedValue / 4095.0;
      float curved = normalized * normalized;
      long rampMs = pedal.rampMinSpeedMs +
                    (curved * (pedal.rampMaxSpeedMs - pedal.rampMinSpeedMs));

      if(pot.pin == POT1_PIN) {
        pedal.modulator.setRampTimeUp(rampMs);
      } else if(pot.pin == POT2_PIN) {
        pedal.modulator.setRampTimeDown(rampMs);
      }
      displayCallback(pot.name, false, midiScaled, rampMs);
    }
  }
}