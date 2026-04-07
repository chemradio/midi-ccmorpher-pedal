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
                                 void (*displayFSChannel)(FSButton &)) {
  bool reading = digitalRead(encBtn.pin);
  unsigned long now = millis();

  // ── Release ────────────────────────────────────────────────────────────────
  if(reading == HIGH && encBtnPressing) {
    if((now - encBtn.lastDebounce) < DEBOUNCE_DELAY) return;
    encBtnPressing = false;
    encBtn.lastDebounce = now;

    if(pedal.inChannelSelect) {
      // Exit channel-select: show the normal FS press screen
      pedal.inChannelSelect = false;
      markStateDirty();
      displayModeChange(pedal.buttons[pedal.channelSelectIdx]);
      return;
    }

    if(!encBtnHandled) {
      // Short tap → cycle footswitch mode
      if(pedal.settingsLocked) { displayLockedMessage("encBtn"); return; }
      int8_t activeIdx = pedal.getActiveButtonIndex();
      if(activeIdx >= 0) {
        FSButton &btn = pedal.buttons[activeIdx];
        btn.toggleFootswitchMode(modulator);
        markStateDirty();
        displayModeChange(btn);
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
  if(reading == LOW && encBtnPressing && !encBtnHandled && !pedal.inChannelSelect) {
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
