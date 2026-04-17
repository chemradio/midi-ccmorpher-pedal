#pragma once
#include "../clock/midiClock.h"
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
    void (*displayCallback)(String, bool, uint8_t, uint32_t),
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
  //
  // Dual-range mapping:
  //   ADC 0–2047    → exponential ms, 0–5000 ms  (plain value, bit 31 = 0)
  //   ADC 2048–4095 → note values 0–16           (CLOCK_SYNC_FLAG | noteIndex)
  int8_t activeIdx = pedal.getActiveButtonIndex();
  if(activeIdx >= 0 && pedal.buttons[activeIdx].isModSwitch) {
    FSButton &btn = pedal.buttons[activeIdx];
    uint16_t adcVal = pot.lastValue;

    uint32_t rampRaw;
    if(adcVal < 2048) {
      float norm = adcVal / 2047.0f;
      rampRaw = (uint32_t)(5000.0f * norm * norm);   // exponential 0–5000 ms
    } else {
      uint8_t noteIdx = (uint8_t)((uint32_t)(adcVal - 2048) * NUM_NOTE_VALUES / 2048);
      if(noteIdx >= NUM_NOTE_VALUES) noteIdx = NUM_NOTE_VALUES - 1;
      rampRaw = CLOCK_SYNC_FLAG | noteIdx;
    }

    unsigned long effectiveMs = (rampRaw & CLOCK_SYNC_FLAG)
                                ? midiClock.syncToMs(rampRaw)
                                : (unsigned long)rampRaw;

    if(pot.pin == POT1_PIN) {
      btn.rampUpMs = rampRaw;
      pedal.modulator.setRampTimeUp(effectiveMs);
      markStateDirty();
      displayCallback(String(btn.name) + " UP", false, 0, rampRaw);
    } else {
      btn.rampDownMs = rampRaw;
      pedal.modulator.setRampTimeDown(effectiveMs);
      markStateDirty();
      displayCallback(String(btn.name) + " DN", false, 0, rampRaw);
    }
  } else {
    // ── Send CC mode ──────────────────────────────────────────────────────
    if(pot.midiCCNumber == POT_CC_OFF) return;
    uint8_t midiScaled = (uint8_t)((pot.lastValue * 128UL) / 4096);
    if(midiScaled != pot.lastMidiValue) {
      pot.lastMidiValue       = midiScaled;
      pot.ccDisplayDirty      = true;
      pot.ccLastDisplayDirty  = millis();
      pot.sendMidiCC(pedal.midiChannel);
      displayCallback("CC " + String(pot.midiCCNumber), true, midiScaled, 0);
    }
  }
}
