#pragma once
#include "../config.h"
#include "../footswitches/footswitchObject.h"
#include "../pedalState.h"
#include "../statePersistance.h"
#include "../menu/mainMenu.h"

inline FSButton encBtn = {ENC_BTN, NO_LED_PIN, "Enc BTN", 0};

inline void initEncoderButton() {
  pinMode(encBtn.pin, INPUT_PULLUP);
}

inline bool          encBtnPressing   = false;
inline bool          encBtnHandled    = false;
inline unsigned long encBtnPressStart = 0;

// displayModeSelect(fsName, catIdx, level, idx1, idx2)
//   level 0: catIdx=highlighted cat; idx1/idx2 unused
//   level 1: idx1=sub-group or variant; idx2 unused
//   level 2: sub-groups→idx1=sub-group/idx2=variant | CC→idx1=Hi value
//   level 3: CC only → idx1=confirmed Hi, idx2=Lo value being set
inline void handleEncoderButton(PedalState &pedal,
                                 void (*displayModeChange)(FSButton &),
                                 MidiCCModulator &modulator,
                                 void (*displayLockedMessage)(String),
                                 void (*displayFSChannel)(FSButton &),
                                 void (*displayModeSelect)(const char *, uint8_t, uint8_t, uint8_t, uint8_t)) {
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
      // ── Main menu ─────────────────────────────────────────────────────────
      if(pedal.menuState != MenuState::NONE) {
        handleMenuPress(pedal);
        return;
      }

      if(pedal.inModeSelect) {
        FSButton &btn = pedal.buttons[pedal.modeSelectFSIdx];
        const ModeCategory &cat = modeCategories[pedal.modeSelectCatIdx];

        if(pedal.modeSelectLevel == 0) {
          // ── Confirm category ──────────────────────────────────────────────
          if(cat.autoSelect) {
            applyModeIndex(btn, cat.firstIdx, &modulator);
            markStateDirty();
            pedal.inModeSelect = false;
            displayModeChange(btn);
          } else {
            pedal.modeSelectLevel = 1;
            if(btn.modeIndex >= cat.firstIdx &&
               btn.modeIndex <  cat.firstIdx + cat.count) {
              uint8_t offset = btn.modeIndex - cat.firstIdx;
              if(cat.subGroupCount > 0) {
                pedal.modeSelectVarIdx    = offset / cat.subGroupSize;
                pedal.modeSelectSubVarIdx = offset % cat.subGroupSize;
              } else {
                pedal.modeSelectVarIdx    = offset;
                pedal.modeSelectSubVarIdx = 0;
              }
            } else {
              pedal.modeSelectVarIdx    = 0;
              pedal.modeSelectSubVarIdx = 0;
            }
            displayModeSelect(btn.name, pedal.modeSelectCatIdx, 1,
                              pedal.modeSelectVarIdx, 0);
          }

        } else if(pedal.modeSelectLevel == 1) {
          // ── Confirm sub-group or variant ──────────────────────────────────
          if(cat.subGroupCount > 0) {
            // Has sub-groups → go to variant level (level 2)
            pedal.modeSelectLevel = 2;
            displayModeSelect(btn.name, pedal.modeSelectCatIdx, 2,
                              pedal.modeSelectVarIdx, pedal.modeSelectSubVarIdx);
          } else {
            // Flat category: check if it's a CC mode → add Hi/Lo config levels
            uint8_t tentIdx = cat.firstIdx + pedal.modeSelectVarIdx;
            bool isCCVariant = (tentIdx < NUM_MODES &&
                                (modes[tentIdx].mode == FootswitchMode::MomentaryCC ||
                                 modes[tentIdx].mode == FootswitchMode::LatchingCC));
            if(isCCVariant) {
              // Save chosen variant, enter Hi config
              pedal.modeSelectCCVariant = pedal.modeSelectVarIdx;
              pedal.modeSelectVarIdx    = btn.ccHigh;  // pre-fill with current Hi
              pedal.modeSelectLevel     = 2;
              displayModeSelect(btn.name, pedal.modeSelectCatIdx, 2,
                                pedal.modeSelectVarIdx, 0);
            } else {
              // All other flat categories: apply immediately
              applyModeIndex(btn, tentIdx, &modulator);
              markStateDirty();
              pedal.inModeSelect = false;
              displayModeChange(btn);
            }
          }

        } else if(pedal.modeSelectLevel == 2) {
          if(cat.subGroupCount > 0) {
            // ── Level 2: confirm variant within sub-group → apply ─────────
            uint8_t newIdx = cat.firstIdx
                           + pedal.modeSelectVarIdx    * cat.subGroupSize
                           + pedal.modeSelectSubVarIdx;
            applyModeIndex(btn, newIdx, &modulator);
            markStateDirty();
            pedal.inModeSelect = false;
            displayModeChange(btn);
          } else {
            // ── Level 2 CC: Hi confirmed → enter Lo config ────────────────
            pedal.modeSelectSubVarIdx = btn.ccLow;   // pre-fill with current Lo
            pedal.modeSelectLevel     = 3;
            displayModeSelect(btn.name, pedal.modeSelectCatIdx, 3,
                              pedal.modeSelectVarIdx, pedal.modeSelectSubVarIdx);
          }

        } else {
          // ── Level 3 CC: Lo confirmed → apply mode + Hi + Lo ──────────────
          uint8_t newIdx = cat.firstIdx + pedal.modeSelectCCVariant;
          applyModeIndex(btn, newIdx, &modulator);
          btn.ccHigh = pedal.modeSelectVarIdx;
          btn.ccLow  = pedal.modeSelectSubVarIdx;
          markStateDirty();
          pedal.inModeSelect = false;
          displayModeChange(btn);
        }

      } else {
        int8_t activeIdx = pedal.getActiveButtonIndex();
        if(activeIdx >= 0) {
          // ── Enter mode select ──────────────────────────────────────────────
          if(pedal.settingsLocked) { displayLockedMessage("encBtn"); return; }
          FSButton &btn = pedal.buttons[activeIdx];
          pedal.inModeSelect        = true;
          pedal.modeSelectLevel     = 0;
          pedal.modeSelectFSIdx     = activeIdx;
          pedal.modeSelectCatIdx    = categoryForModeIndex(btn.modeIndex);
          pedal.modeSelectVarIdx    = 0;
          pedal.modeSelectSubVarIdx = 0;
          displayModeSelect(btn.name, pedal.modeSelectCatIdx, 0, 0, 0);
        } else if(!pedal.settingsLocked) {
          // ── Enter main menu ────────────────────────────────────────────────
          pedal.menuState   = MenuState::ROOT;
          pedal.menuItemIdx = 0;
          displayMenuRoot(pedal);
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
    if(pedal.settingsLocked && pedal.getActiveButtonIndex() < 0) {
      displayUnlockProgress(0);
    }
    return;
  }

  // ── Held ────────────────────────────────────────────────────────────────────
  if(reading == LOW && encBtnPressing && !encBtnHandled) {
    int8_t activeIdx = pedal.getActiveButtonIndex();

    if(pedal.settingsLocked && activeIdx < 0) {
      unsigned long held = now - encBtnPressStart;
      if(held >= UNLOCK_HOLD_MS) {
        pedal.settingsLocked = false;
        encBtnHandled        = true;
        displayLockChange(false);
      } else {
        static unsigned long lastProgressUpdate = 0;
        if(now - lastProgressUpdate > 80) {
          lastProgressUpdate = now;
          displayUnlockProgress((uint8_t)(held * 100UL / UNLOCK_HOLD_MS));
        }
      }
      return;
    }

    if(pedal.menuState == MenuState::ROUTING
       && (now - encBtnPressStart) >= CHANNEL_SELECT_HOLD_MS) {
      handleMenuLongPress(pedal);
      encBtnHandled = true;
      return;
    }

    if(!pedal.inChannelSelect && !pedal.inModeSelect
       && pedal.menuState == MenuState::NONE
       && activeIdx >= 0
       && !pedal.buttons[activeIdx].isKeyboard
       && (now - encBtnPressStart) >= CHANNEL_SELECT_HOLD_MS) {
      pedal.inChannelSelect  = true;
      pedal.channelSelectIdx = activeIdx;
      encBtnHandled          = true;
      displayFSChannel(pedal.buttons[activeIdx]);
    }
  }
}
