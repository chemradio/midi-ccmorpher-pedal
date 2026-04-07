#pragma once
#include "../config.h"
#include "../pedalState.h"
#include "../potentiometers/analogReadHelpers.h"
#include "../sharedTypes.h"

inline AnalogPot analogPots[] = {
  {POT1_PIN, "UP",   POT1_CC},
  {POT2_PIN, "DOWN", POT2_CC},
};

inline void initAnalogPots() {
  for(auto &pot : analogPots) {
    pinMode(pot.pin, INPUT);
  }
  // EMA seeds itself on the first update() call — no pre-read needed.
}

// Throttle for the "settings locked" display message.
inline unsigned long lastLockedPotMsg  = 0;
inline constexpr unsigned long LOCKED_POT_MSG_INTERVAL = 1000;

inline void handleAnalogPot(
    AnalogPot &pot,
    PedalState &pedal,
    void (*displayCallback)(String, bool, uint8_t, long),
    void (*displayLockedMessage)(String)) {

  if(!pot.update()) return; // filtered value hasn't moved enough — do nothing

  // ── Settings locked ───────────────────────────────────────────────────────
  if(pedal.settingsLocked) {
    unsigned long now = millis();
    if(now - lastLockedPotMsg > LOCKED_POT_MSG_INTERVAL) {
      lastLockedPotMsg = now;
      displayLockedMessage("pots");
    }
    return;
  }

  // ── Ramp speed mode ───────────────────────────────────────────────────────
  // Active while a mod-switch footswitch is held. Adjusts that footswitch's
  // stored ramp time and applies it to the modulator immediately.
  int8_t activeIdx = pedal.getActiveButtonIndex();
  if(activeIdx >= 0 && pedal.buttons[activeIdx].isModSwitch) {
    FSButton &btn = pedal.buttons[activeIdx];

    // Exponential pot feel: small movements at the low end, larger at the top.
    float normalized = pot.lastValue / 4095.0f;
    long  rampMs     = pedal.rampMinSpeedMs +
                       (long)(normalized * normalized *
                              (float)(pedal.rampMaxSpeedMs - pedal.rampMinSpeedMs));

    uint8_t midiScaled = (uint8_t)((pot.lastValue * 128UL) / 4096);

    if(pot.pin == POT1_PIN) {
      btn.rampUpMs = rampMs;
      pedal.modulator.setRampTimeUp(rampMs);
      markStateDirty();
      displayCallback(String(btn.name) + " UP", false, midiScaled, rampMs);
    } else {
      btn.rampDownMs = rampMs;
      pedal.modulator.setRampTimeDown(rampMs);
      markStateDirty();
      displayCallback(String(btn.name) + " DN", false, midiScaled, rampMs);
    }
  } else {
    // ── Send CC mode ──────────────────────────────────────────────────────
    // Default: pot acts as a direct CC controller.
    uint8_t midiScaled = (uint8_t)((pot.lastValue * 128UL) / 4096);
    if(midiScaled != pot.lastMidiValue) {
      pot.lastMidiValue = midiScaled;
      pot.sendMidiCC(pedal.midiChannel);
      displayCallback("CC " + String(pot.midiCCNumber), true, midiScaled, 0);
    }
  }
}
