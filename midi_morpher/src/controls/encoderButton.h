#pragma once
#include "../config.h"
#include "../footswitches/footswitchObject.h"
#include "../pedalState.h"
#include "../statePersistance.h"

inline FSButton encBtn = {ENC_BTN, NO_LED_PIN, "Enc BTN", 0};

inline void initEncoderButton() {
  pinMode(encBtn.pin, INPUT_PULLUP);
}

// State for tap vs. long-press detection.
inline bool          encBtnPressing   = false;
inline bool          encBtnHandled    = false; // true once long-press action has fired
inline unsigned long encBtnPressStart = 0;

inline void handleEncoderButton(PedalState &pedal,
                                 void (*displayModeChange)(FSButton &),
                                 MidiCCModulator &modulator,
                                 void (*displayLockedMessage)(String),
                                 void (*displayFSChannel)(FSButton &),
                                 void (*displayModeSelect)(const char *, uint8_t, uint8_t, bool)) {
  bool reading = digitalRead(encBtn.pin);
  unsigned long now = millis();

  // ── Release ────────────────────────────────────────────────────────────────
  if(reading == HIGH && encBtnPressing) {
    if((now - encBtn.lastDebounce) < DEBOUNCE_DELAY) return;
    encBtnPressing = false;
    encBtn.lastDebounce = now;

    if(pedal.inChannelSelect) {
      pedal.inChannelSelect = false;
      markStateDirty();
      displayModeChange(pedal.buttons[pedal.channelSelectIdx]);
      return;
    }

    if(!encBtnHandled) {
      if(pedal.inModeSelect) {
        // ── Mode-select state machine ────────────────────────────────────────
        if(pedal.modeSelectLevel == 0) {
          // Drill into variant select
          pedal.modeSelectLevel = 1;
          const ModeCategory &cat = modeCategories[pedal.modeSelectCatIdx];
          FSButton &btn = pedal.buttons[pedal.modeSelectFSIdx];
          // If current mode is already in this category, start at that variant
          if(btn.modeIndex >= cat.firstIdx && btn.modeIndex < cat.firstIdx + cat.count)
            pedal.modeSelectVarIdx = btn.modeIndex - cat.firstIdx;
          else
            pedal.modeSelectVarIdx = 0;
          displayModeSelect(btn.name, pedal.modeSelectCatIdx, pedal.modeSelectVarIdx, true);
        } else {
          // Confirm — apply the selected mode
          const ModeCategory &cat = modeCategories[pedal.modeSelectCatIdx];
          uint8_t newModeIdx = cat.firstIdx + pedal.modeSelectVarIdx;
          FSButton &btn = pedal.buttons[pedal.modeSelectFSIdx];
          applyModeIndex(btn, newModeIdx, &modulator);
          markStateDirty();
          pedal.inModeSelect = false;
          displayModeChange(btn);
        }
      } else {
        // ── Enter mode select ────────────────────────────────────────────────
        if(pedal.settingsLocked) { displayLockedMessage("encBtn"); return; }
        int8_t activeIdx = pedal.getActiveButtonIndex();
        if(activeIdx >= 0) {
          pedal.inModeSelect     = true;
          pedal.modeSelectLevel  = 0;
          pedal.modeSelectFSIdx  = activeIdx;
          FSButton &btn = pedal.buttons[activeIdx];
          pedal.modeSelectCatIdx = categoryForModeIndex(btn.modeIndex);
          displayModeSelect(btn.name, pedal.modeSelectCatIdx, 0, false);
        }
      }
    }
    return;
  }

  // ── Press start ────────────────────────────────────────────────────────────
  if(reading == LOW && !encBtnPressing) {
    if((now - encBtn.lastDebounce) < DEBOUNCE_DELAY) return;
    encBtnPressing   = true;
    encBtnHandled    = false;
    encBtnPressStart = now;
    encBtn.lastDebounce = now;
    return;
  }

  // ── Held — poll for long-press threshold ───────────────────────────────────
  // Skip if already in channel-select or mode-select.
  if(reading == LOW && encBtnPressing && !encBtnHandled
     && !pedal.inChannelSelect && !pedal.inModeSelect) {
    int8_t activeIdx = pedal.getActiveButtonIndex();
    if(activeIdx >= 0 && (now - encBtnPressStart) >= CHANNEL_SELECT_HOLD_MS) {
      if(pedal.settingsLocked) { displayLockedMessage("encBtn"); encBtnHandled = true; return; }
      pedal.inChannelSelect  = true;
      pedal.channelSelectIdx = activeIdx;
      encBtnHandled          = true;
      displayFSChannel(pedal.buttons[activeIdx]);
    }
  }
}
