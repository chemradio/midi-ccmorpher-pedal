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
                                 void (*displayLockedMessage)(String),
                                 void (*displayFSChannel)(FSButton &),
                                 void (*displayModeSelect)(const char *, uint8_t, uint8_t, uint8_t, uint8_t),
                                 void (*displayActionSelect)(FSButton &, uint8_t)) {
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

      if(pedal.inActionSelect) {
        FSButton &btn = pedal.buttons[pedal.modeSelectFSIdx];
        bool expanded = btn.extraActions[0].enabled || btn.extraActions[1].enabled || btn.extraActions[2].enabled;
        uint8_t slot = pedal.actionSelectSlot;

        if(!expanded && slot == 1) {
          // +EXPAND: enable all 3 extra actions with NoAction mode
          uint8_t noActIdx = 0; // NoAction is index 0
          for(uint8_t t = 0; t < FS_NUM_EXTRA; t++) {
            btn.extraActions[t].enabled   = true;
            btn.extraActions[t].modeIndex = noActIdx;
          }
          markStateDirty();
          pedal.actionSelectSlot = 0;
          displayActionSelect(btn, 0);
        } else if(expanded && slot == 4) {
          // COLLAPSE: disable all extra actions
          for(uint8_t t = 0; t < FS_NUM_EXTRA; t++)
            btn.extraActions[t].enabled = false;
          markStateDirty();
          pedal.actionSelectSlot = 0;
          displayActionSelect(btn, 0);
        } else if(slot <= 3) {
          // Enter mode select for the chosen slot
          // slot 0=PRESS(-1), slot 1=RELEASE(t=2), slot 2=HOLD(t=0), slot 3=DBL(t=1)
          int8_t extraType;
          if(slot == 0)      extraType = -1;
          else if(slot == 1) extraType = 2;
          else if(slot == 2) extraType = 0;
          else               extraType = 1;

          uint8_t curModeIdx = (extraType < 0)
              ? btn.modeIndex
              : btn.extraActions[(uint8_t)extraType].modeIndex;

          pedal.modeSelectFromActionSelect = true;
          pedal.modeSelectExtraActionType  = extraType;
          pedal.inActionSelect             = false;
          pedal.inModeSelect               = true;
          pedal.modeSelectLevel            = 0;
          pedal.modeSelectCatIdx           = categoryForModeIndex(curModeIdx);
          pedal.modeSelectVarIdx           = 0;
          pedal.modeSelectSubVarIdx        = 0;
          displayModeSelect(btn.name, pedal.modeSelectCatIdx, 0, 0, 0);
        }
        return;
      }

      if(pedal.inModeSelect) {
        FSButton &btn = pedal.buttons[pedal.modeSelectFSIdx];
        const ModeCategory &cat = modeCategories[pedal.modeSelectCatIdx];

        if(pedal.modeSelectLevel == 0) {
          // ── Confirm category ──────────────────────────────────────────────
          if(cat.autoSelect) {
            int8_t _xt = pedal.modeSelectExtraActionType;
            if(pedal.modeSelectFromActionSelect && _xt >= 0) {
              btn.extraActions[_xt].modeIndex = cat.firstIdx;
              btn.extraActions[_xt].enabled   = true;
            } else {
              applyModeIndex(btn, cat.firstIdx, &pedal.modForFS(pedal.modeSelectFSIdx));
            }
            markStateDirty();
            pedal.inModeSelect = false;
            if(pedal.modeSelectFromActionSelect) {
              pedal.modeSelectFromActionSelect = false;
              pedal.inActionSelect = true;
              displayActionSelect(btn, pedal.actionSelectSlot);
            } else {
              displayModeChange(btn);
            }
          } else {
            // Multi category: jump to scene-select level, or bail if no scenes
            bool isMultiCat = (cat.firstIdx < NUM_MODES &&
                               modes[cat.firstIdx].mode == FootswitchMode::Multi);
            if(isMultiCat) {
              uint8_t fs = firstMultiSlot();
              if(fs == 0xFF) {
                pedal.inModeSelect = false;
                displayNoMultisMessage(btn.name);
                return;
              }
              pedal.modeSelectVarIdx = (btn.mode == FootswitchMode::Multi &&
                                        btn.midiNumber < MAX_MULTI_SCENES &&
                                        multiScenes[btn.midiNumber].name[0] != '\0')
                                        ? btn.midiNumber : fs;
              pedal.modeSelectLevel = 1;
              displayModeSelect(btn.name, pedal.modeSelectCatIdx, 1,
                                pedal.modeSelectVarIdx, 0);
              return;
            }
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
          // Multi: apply mode and bind the selected scene slot
          bool isMultiCat = (cat.firstIdx < NUM_MODES &&
                             modes[cat.firstIdx].mode == FootswitchMode::Multi);
          if(isMultiCat) {
            int8_t _xt = pedal.modeSelectExtraActionType;
            if(pedal.modeSelectFromActionSelect && _xt >= 0) {
              btn.extraActions[_xt].modeIndex  = cat.firstIdx;
              btn.extraActions[_xt].midiNumber = pedal.modeSelectVarIdx;
              btn.extraActions[_xt].enabled    = true;
            } else {
              applyModeIndex(btn, cat.firstIdx, &pedal.modForFS(pedal.modeSelectFSIdx));
              btn.midiNumber = pedal.modeSelectVarIdx;
            }
            markStateDirty();
            pedal.inModeSelect = false;
            if(pedal.modeSelectFromActionSelect) {
              pedal.modeSelectFromActionSelect = false;
              pedal.inActionSelect = true;
              displayActionSelect(btn, pedal.actionSelectSlot);
            } else {
              displayModeChange(btn);
            }
            return;
          }
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
                                 modes[tentIdx].mode == FootswitchMode::LatchingCC  ||
                                 modes[tentIdx].mode == FootswitchMode::SingleCC));
            if(isCCVariant) {
              // Save chosen variant, enter value config
              pedal.modeSelectCCVariant = pedal.modeSelectVarIdx;
              int8_t _xt2 = pedal.modeSelectExtraActionType;
              pedal.modeSelectVarIdx = (pedal.modeSelectFromActionSelect && _xt2 >= 0)
                  ? btn.extraActions[_xt2].ccHigh : btn.ccHigh;
              pedal.modeSelectLevel     = 2;
              displayModeSelect(btn.name, pedal.modeSelectCatIdx, 2,
                                pedal.modeSelectVarIdx, pedal.modeSelectCCVariant);
            } else {
              // All other flat categories: apply immediately
              int8_t _xt = pedal.modeSelectExtraActionType;
              if(pedal.modeSelectFromActionSelect && _xt >= 0) {
                btn.extraActions[_xt].modeIndex = tentIdx;
                btn.extraActions[_xt].enabled   = true;
              } else {
                applyModeIndex(btn, tentIdx, &pedal.modForFS(pedal.modeSelectFSIdx));
              }
              markStateDirty();
              pedal.inModeSelect = false;
              if(pedal.modeSelectFromActionSelect) {
                pedal.modeSelectFromActionSelect = false;
                pedal.inActionSelect = true;
                displayActionSelect(btn, pedal.actionSelectSlot);
              } else {
                displayModeChange(btn);
              }
            }
          }

        } else if(pedal.modeSelectLevel == 2) {
          if(cat.subGroupCount > 0) {
            // ── Level 2: confirm variant within sub-group → apply ─────────
            uint8_t newIdx = cat.firstIdx
                           + pedal.modeSelectVarIdx    * cat.subGroupSize
                           + pedal.modeSelectSubVarIdx;
            int8_t _xt = pedal.modeSelectExtraActionType;
            if(pedal.modeSelectFromActionSelect && _xt >= 0) {
              btn.extraActions[_xt].modeIndex = newIdx;
              btn.extraActions[_xt].enabled   = true;
            } else {
              applyModeIndex(btn, newIdx, &pedal.modForFS(pedal.modeSelectFSIdx));
            }
            markStateDirty();
            pedal.inModeSelect = false;
            if(pedal.modeSelectFromActionSelect) {
              pedal.modeSelectFromActionSelect = false;
              pedal.inActionSelect = true;
              displayActionSelect(btn, pedal.actionSelectSlot);
            } else {
              displayModeChange(btn);
            }
          } else {
            uint8_t ccModeIdx = cat.firstIdx + pedal.modeSelectCCVariant;
            if(ccModeIdx < NUM_MODES && modes[ccModeIdx].mode == FootswitchMode::SingleCC) {
              // ── Level 2 Single CC: value confirmed → apply ────────────────
              int8_t _xt = pedal.modeSelectExtraActionType;
              if(pedal.modeSelectFromActionSelect && _xt >= 0) {
                btn.extraActions[_xt].modeIndex = ccModeIdx;
                btn.extraActions[_xt].ccHigh    = pedal.modeSelectVarIdx;
                btn.extraActions[_xt].enabled   = true;
              } else {
                applyModeIndex(btn, ccModeIdx, &pedal.modForFS(pedal.modeSelectFSIdx));
                btn.ccHigh = pedal.modeSelectVarIdx;
              }
              markStateDirty();
              pedal.inModeSelect = false;
              if(pedal.modeSelectFromActionSelect) {
                pedal.modeSelectFromActionSelect = false;
                pedal.inActionSelect = true;
                displayActionSelect(btn, pedal.actionSelectSlot);
              } else {
                displayModeChange(btn);
              }
            } else {
              // ── Level 2 CC: Hi confirmed → enter Lo config ────────────────
              int8_t _xt3 = pedal.modeSelectExtraActionType;
              pedal.modeSelectSubVarIdx = (pedal.modeSelectFromActionSelect && _xt3 >= 0)
                  ? btn.extraActions[_xt3].ccLow : btn.ccLow;
              pedal.modeSelectLevel     = 3;
              displayModeSelect(btn.name, pedal.modeSelectCatIdx, 3,
                                pedal.modeSelectVarIdx, pedal.modeSelectSubVarIdx);
            }
          }

        } else {
          // ── Level 3 CC: Lo confirmed → apply mode + Hi + Lo ──────────────
          uint8_t newIdx = cat.firstIdx + pedal.modeSelectCCVariant;
          int8_t _xt = pedal.modeSelectExtraActionType;
          if(pedal.modeSelectFromActionSelect && _xt >= 0) {
            btn.extraActions[_xt].modeIndex = newIdx;
            btn.extraActions[_xt].ccHigh    = pedal.modeSelectVarIdx;
            btn.extraActions[_xt].ccLow     = pedal.modeSelectSubVarIdx;
            btn.extraActions[_xt].enabled   = true;
          } else {
            applyModeIndex(btn, newIdx, &pedal.modForFS(pedal.modeSelectFSIdx));
            btn.ccHigh = pedal.modeSelectVarIdx;
            btn.ccLow  = pedal.modeSelectSubVarIdx;
          }
          markStateDirty();
          pedal.inModeSelect = false;
          if(pedal.modeSelectFromActionSelect) {
            pedal.modeSelectFromActionSelect = false;
            pedal.inActionSelect = true;
            displayActionSelect(btn, pedal.actionSelectSlot);
          } else {
            displayModeChange(btn);
          }
        }

      } else {
        int8_t activeIdx = pedal.getActiveButtonIndex();
        if(activeIdx >= 0) {
          // ── Enter action select ────────────────────────────────────────────
          if(pedal.settingsLocked) { displayLockedMessage("encBtn"); return; }
          FSButton &btn = pedal.buttons[activeIdx];
          pedal.inActionSelect             = true;
          pedal.actionSelectSlot           = 0;
          pedal.modeSelectFSIdx            = activeIdx;
          pedal.modeSelectFromActionSelect = false;
          displayActionSelect(btn, 0);
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

      // ── FS + enc btn in menu → exit immediately ────────────────
 if(pedal.menuState != MenuState::NONE && activeIdx >= 0) {
  pedal.menuState = MenuState::NONE;
  displayHomeScreen(pedal);
  lastInteraction = millis();
  return;
}

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

    if(!pedal.inChannelSelect && !pedal.inModeSelect && !pedal.inActionSelect
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
