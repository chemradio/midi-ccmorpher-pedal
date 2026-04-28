#pragma once
#include "../config.h"
#include "../footswitches/footswitchObject.h"
#include "../menu/mainMenu.h"
#include "../pedalState.h"
#include "../statePersistance.h"

inline FSButton encBtn = {ENC_BTN, "Enc BTN", 0};

inline void initEncoderButton() {
  pinMode(encBtn.pin, INPUT_PULLUP);
}

inline FSButton &_modeSelectBtn(PedalState &pedal) { return pedal.modeSelectBtn(); }
inline MidiCCModulator &_modeSelectMod(PedalState &pedal) { return pedal.modeSelectMod(); }

// If extra action slot xt currently holds a modulation mode, stop it immediately.
inline void _stopExtraModIfNeeded(FSButton &btn, int8_t xt, MidiCCModulator &mod) {
  if(xt < 0 || (uint8_t)xt >= FS_NUM_EXTRA)
    return;
  uint8_t mi = btn.extraActions[(uint8_t)xt].modeIndex;
  if(mi < NUM_MODES && modes[mi].isModSwitch)
    mod.reset();
}

inline void _saveLoadAction(PedalState &pedal) {
  const FSButton &lb = pedal.loadActionEditBtn;
  FSActionPersisted &la = presets[activePreset].loadAction;
  la.enabled = (lb.modeIndex != 0);
  la.modeIndex = lb.modeIndex;
  la.midiNumber = lb.midiNumber;
  la.fsChannel = lb.fsChannel;
  la.ccLow = lb.ccLow;
  la.ccHigh = lb.ccHigh;
  la.velocity = lb.velocity;
  la.rampUpMs = lb.rampUpMs;
  la.rampDownMs = lb.rampDownMs;
  la._pad = 0;
  markStateDirty();
}

// After applying mode (and optionally Hi/Lo for CC), transition to value entry
// (level 4) or exit mode select if no value is needed.
// newIdx       = the fully-determined mode index
// isExtraAction = true if editing an extra action slot
// Returns true if entering level 4; caller should return after this.
inline bool _transitionToValueEntry(
    PedalState &pedal, FSButton &btn, uint8_t newIdx, bool isExtraAction, void (*displayModeSelect)(const char *, uint8_t, uint8_t, uint8_t, uint8_t), void (*displayModeChange)(FSButton &), void (*displayActionSelect)(FSButton &, uint8_t)) {
  if(!modeNeedsValueEntry(newIdx)) {
    // No value entry needed — exit mode select
    markStateDirty();
    pedal.inModeSelect = false;
    if(isExtraAction) {
      pedal.modeSelectFromActionSelect = false;
      pedal.inActionSelect = true;
      displayActionSelect(btn, pedal.actionSelectSlot);
    } else if(pedal.modeSelectFSIdx == FS_LOAD_ACTION_IDX) {
      _saveLoadAction(pedal);
      pedal.menuState = MenuState::ROOT;
      pedal.menuItemIdx = MENU_PRESET_ACTION;
      displayMenuRoot(pedal);
    } else {
      displayModeChange(btn);
    }
    return false;
  }
  // Enter level 4
  pedal.modeSelectFinalIdx = newIdx;
  uint8_t curMidi = isExtraAction
                        ? btn.extraActions[(uint8_t)pedal.modeSelectExtraActionType].midiNumber
                        : btn.midiNumber;
  // Clamp initial value to valid range
  if(newIdx < NUM_MODES) {
    const ModeInfo &mi = modes[newIdx];
    if(mi.isSystem)
      curMidi = min(curMidi, (uint8_t)(NUM_SYS_CMDS - 1));
    else if(mi.isKeyboard)
      curMidi = min(curMidi, (uint8_t)(NUM_HID_KEYS - 1));
    else if(mi.isScene)
      curMidi = min(curMidi, mi.sceneMaxVal);
    else if(mi.mode == FootswitchMode::PresetNum) {
      uint8_t maxP = pedal.globalSettings.presetCount > 0 ? pedal.globalSettings.presetCount - 1 : 0;
      curMidi = min(curMidi, maxP);
    }
    // modSwitch: PB_SENTINEL is valid, otherwise keep as-is (will be clamped on scroll)
  }
  pedal.modeSelectVarIdx = curMidi;
  pedal.modeSelectLevel = 4;
  displayModeSelect(btn.name, pedal.modeSelectCatIdx, 4, curMidi, newIdx);
  return true;
}

inline bool encBtnPressing = false;
inline bool encBtnHandled = false;
inline bool encBtnDidPresetNav = false;
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
                                void (*displayActionSelect)(FSButton &, uint8_t),
                                void (*onPresetSave)()) {
  bool reading = digitalRead(encBtn.pin);
  unsigned long now = millis();

  // ── Release ────────────────────────────────────────────────────────────────
  if(reading == HIGH && encBtnPressing) {
    if((now - encBtn.lastDebounce) < DEBOUNCE_DELAY)
      return;
    encBtnPressing = false;
    encBtn.lastDebounce = now;

    if(pedal.inChannelSelect) {
      pedal.inChannelSelect = false;
      markStateDirty();
      displayModeChange(pedal.buttons[pedal.channelSelectIdx]);
      return;
    }

    if(!encBtnHandled) {
      if(pedal.inFSEdit) {
        pedal.fsEditEditing = !pedal.fsEditEditing;
        displayFSEditMenu(pedal);
        return;
      }

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
          for(uint8_t t = 0; t < FS_NUM_EXTRA; t++) {
            btn.extraActions[t].enabled = true;
            btn.extraActions[t].modeIndex = 0;
          }
          markStateDirty();
          pedal.actionSelectSlot = 0;
          displayActionSelect(btn, 0);
        } else if(expanded && slot == 4) {
          // COLLAPSE: stop any running modulation, then disable all extra actions
          MidiCCModulator &colMod = _modeSelectMod(pedal);
          for(uint8_t t = 0; t < FS_NUM_EXTRA; t++) {
            if(btn.extraActions[t].enabled &&
               btn.extraActions[t].modeIndex < NUM_MODES &&
               modes[btn.extraActions[t].modeIndex].isModSwitch) {
              colMod.reset();
              break;
            }
          }
          for(uint8_t t = 0; t < FS_NUM_EXTRA; t++)
            btn.extraActions[t].enabled = false;
          markStateDirty();
          pedal.actionSelectSlot = 0;
          displayActionSelect(btn, 0);
        } else if(slot <= 3) {
          // Enter cursor edit menu for chosen slot
          // slot 0=PRESS(-1), slot 1=RELEASE(t=2), slot 2=HOLD(t=0), slot 3=DBL(t=1)
          int8_t extraType;
          if(slot == 0)
            extraType = -1;
          else if(slot == 1)
            extraType = 2;
          else if(slot == 2)
            extraType = 0;
          else
            extraType = 1;

          pedal.inActionSelect = false;
          pedal.inFSEdit = true;
          pedal.fsEditFSIdx = (uint8_t)pedal.modeSelectFSIdx;
          pedal.fsEditExtraType = extraType;
          pedal.fsEditCursor = 0;
          pedal.fsEditEditing = false;

          // Populate loadActionEditBtn for extra action edits
          if(extraType >= 0) {
            const FSAction &act = btn.extraActions[(uint8_t)extraType];
            FSButton &lb = pedal.loadActionEditBtn;
            applyModeIndex(lb, act.modeIndex < NUM_MODES ? act.modeIndex : 0, nullptr);
            lb.midiNumber = act.midiNumber;
            lb.fsChannel = act.fsChannel;
            lb.ccLow = act.ccLow;
            lb.ccHigh = act.ccHigh;
            lb.velocity = act.velocity;
            lb.rampUpMs = act.rampUpMs;
            lb.rampDownMs = act.rampDownMs;
          }
          displayFSEditMenu(pedal);
        }
        return;
      }

      if(pedal.inModeSelect) {
        FSButton &btn = _modeSelectBtn(pedal);
        const ModeCategory &cat = modeCategories[pedal.modeSelectCatIdx];

        if(pedal.modeSelectLevel == 0) {
          // ── Confirm category ──────────────────────────────────────────────
          if(cat.autoSelect) {
            uint8_t finalIdx = cat.firstIdx;
            int8_t _xt = pedal.modeSelectExtraActionType;
            bool isExtra = (pedal.modeSelectFromActionSelect && _xt >= 0);
            if(isExtra) {
              _stopExtraModIfNeeded(btn, _xt, _modeSelectMod(pedal));
              btn.extraActions[_xt].modeIndex = finalIdx;
              btn.extraActions[_xt].enabled = true;
            } else {
              applyModeIndex(btn, finalIdx, &_modeSelectMod(pedal));
            }
            _transitionToValueEntry(pedal, btn, finalIdx, isExtra, displayModeSelect, displayModeChange, displayActionSelect);
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
                                           ? btn.midiNumber
                                           : fs;
              pedal.modeSelectLevel = 1;
              displayModeSelect(btn.name, pedal.modeSelectCatIdx, 1, pedal.modeSelectVarIdx, 0);
              return;
            }
            pedal.modeSelectLevel = 1;
            if(btn.modeIndex >= cat.firstIdx &&
               btn.modeIndex < cat.firstIdx + cat.count) {
              uint8_t offset = btn.modeIndex - cat.firstIdx;
              if(cat.subGroupCount > 0) {
                pedal.modeSelectVarIdx = offset / cat.subGroupSize;
                pedal.modeSelectSubVarIdx = offset % cat.subGroupSize;
              } else {
                pedal.modeSelectVarIdx = offset;
                pedal.modeSelectSubVarIdx = 0;
              }
            } else {
              pedal.modeSelectVarIdx = 0;
              pedal.modeSelectSubVarIdx = 0;
            }
            displayModeSelect(btn.name, pedal.modeSelectCatIdx, 1, pedal.modeSelectVarIdx, 0);
          }

        } else if(pedal.modeSelectLevel == 1) {
          // ── Confirm sub-group or variant ──────────────────────────────────
          // Multi: apply mode and bind the selected scene slot
          bool isMultiCat = (cat.firstIdx < NUM_MODES &&
                             modes[cat.firstIdx].mode == FootswitchMode::Multi);
          if(isMultiCat) {
            int8_t _xt = pedal.modeSelectExtraActionType;
            if(pedal.modeSelectFromActionSelect && _xt >= 0) {
              _stopExtraModIfNeeded(btn, _xt, _modeSelectMod(pedal));
              btn.extraActions[_xt].modeIndex = cat.firstIdx;
              btn.extraActions[_xt].midiNumber = pedal.modeSelectVarIdx;
              btn.extraActions[_xt].enabled = true;
            } else {
              applyModeIndex(btn, cat.firstIdx, &_modeSelectMod(pedal));
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
            displayModeSelect(btn.name, pedal.modeSelectCatIdx, 2, pedal.modeSelectVarIdx, pedal.modeSelectSubVarIdx);
          } else {
            // Flat category: check if it's a CC mode → add Hi/Lo config levels
            uint8_t tentIdx = cat.firstIdx + pedal.modeSelectVarIdx;
            bool isCCVariant = (tentIdx < NUM_MODES &&
                                (modes[tentIdx].mode == FootswitchMode::MomentaryCC ||
                                 modes[tentIdx].mode == FootswitchMode::LatchingCC ||
                                 modes[tentIdx].mode == FootswitchMode::SingleCC));
            if(isCCVariant) {
              // Save chosen variant, enter value config
              pedal.modeSelectCCVariant = pedal.modeSelectVarIdx;
              int8_t _xt2 = pedal.modeSelectExtraActionType;
              pedal.modeSelectVarIdx = (pedal.modeSelectFromActionSelect && _xt2 >= 0)
                                           ? btn.extraActions[_xt2].ccHigh
                                           : btn.ccHigh;
              pedal.modeSelectLevel = 2;
              displayModeSelect(btn.name, pedal.modeSelectCatIdx, 2, pedal.modeSelectVarIdx, pedal.modeSelectCCVariant);
            } else {
              // All other flat categories: apply mode, then enter value entry or exit
              int8_t _xt = pedal.modeSelectExtraActionType;
              bool isExtra = (pedal.modeSelectFromActionSelect && _xt >= 0);
              if(isExtra) {
                _stopExtraModIfNeeded(btn, _xt, _modeSelectMod(pedal));
                btn.extraActions[_xt].modeIndex = tentIdx;
                btn.extraActions[_xt].enabled = true;
              } else {
                applyModeIndex(btn, tentIdx, &_modeSelectMod(pedal));
              }
              _transitionToValueEntry(pedal, btn, tentIdx, isExtra, displayModeSelect, displayModeChange, displayActionSelect);
            }
          }

        } else if(pedal.modeSelectLevel == 2) {
          if(cat.subGroupCount > 0) {
            // ── Level 2: confirm variant within sub-group → apply + value entry ─
            uint8_t newIdx = cat.firstIdx + pedal.modeSelectVarIdx * cat.subGroupSize + pedal.modeSelectSubVarIdx;
            int8_t _xt = pedal.modeSelectExtraActionType;
            bool isExtra = (pedal.modeSelectFromActionSelect && _xt >= 0);
            if(isExtra) {
              _stopExtraModIfNeeded(btn, _xt, _modeSelectMod(pedal));
              btn.extraActions[_xt].modeIndex = newIdx;
              btn.extraActions[_xt].enabled = true;
            } else {
              applyModeIndex(btn, newIdx, &_modeSelectMod(pedal));
            }
            _transitionToValueEntry(pedal, btn, newIdx, isExtra, displayModeSelect, displayModeChange, displayActionSelect);
          } else {
            uint8_t ccModeIdx = cat.firstIdx + pedal.modeSelectCCVariant;
            if(ccModeIdx < NUM_MODES && modes[ccModeIdx].mode == FootswitchMode::SingleCC) {
              // ── Level 2 Single CC: value confirmed → apply + enter CC# ───
              int8_t _xt = pedal.modeSelectExtraActionType;
              bool isExtra = (pedal.modeSelectFromActionSelect && _xt >= 0);
              if(isExtra) {
                _stopExtraModIfNeeded(btn, _xt, _modeSelectMod(pedal));
                btn.extraActions[_xt].modeIndex = ccModeIdx;
                btn.extraActions[_xt].ccHigh = pedal.modeSelectVarIdx;
                btn.extraActions[_xt].enabled = true;
              } else {
                applyModeIndex(btn, ccModeIdx, &_modeSelectMod(pedal));
                btn.ccHigh = pedal.modeSelectVarIdx;
              }
              // Enter level 4 for CC#
              pedal.modeSelectFinalIdx = ccModeIdx;
              uint8_t curCC = isExtra ? btn.extraActions[_xt].midiNumber : btn.midiNumber;
              pedal.modeSelectVarIdx = curCC;
              pedal.modeSelectLevel = 4;
              displayModeSelect(btn.name, pedal.modeSelectCatIdx, 4, curCC, ccModeIdx);
            } else {
              // ── Level 2 CC: Hi confirmed → enter Lo config ────────────────
              int8_t _xt3 = pedal.modeSelectExtraActionType;
              pedal.modeSelectSubVarIdx = (pedal.modeSelectFromActionSelect && _xt3 >= 0)
                                              ? btn.extraActions[_xt3].ccLow
                                              : btn.ccLow;
              pedal.modeSelectLevel = 3;
              displayModeSelect(btn.name, pedal.modeSelectCatIdx, 3, pedal.modeSelectVarIdx, pedal.modeSelectSubVarIdx);
            }
          }

        } else if(pedal.modeSelectLevel == 3) {
          // ── Level 3 CC: Lo confirmed → apply mode + Hi + Lo → enter CC# ──
          uint8_t newIdx = cat.firstIdx + pedal.modeSelectCCVariant;
          int8_t _xt = pedal.modeSelectExtraActionType;
          bool isExtra = (pedal.modeSelectFromActionSelect && _xt >= 0);
          if(isExtra) {
            _stopExtraModIfNeeded(btn, _xt, _modeSelectMod(pedal));
            btn.extraActions[_xt].modeIndex = newIdx;
            btn.extraActions[_xt].ccHigh = pedal.modeSelectVarIdx;
            btn.extraActions[_xt].ccLow = pedal.modeSelectSubVarIdx;
            btn.extraActions[_xt].enabled = true;
          } else {
            applyModeIndex(btn, newIdx, &_modeSelectMod(pedal));
            btn.ccHigh = pedal.modeSelectVarIdx;
            btn.ccLow = pedal.modeSelectSubVarIdx;
          }
          // Enter level 4 for CC#
          pedal.modeSelectFinalIdx = newIdx;
          uint8_t curCC = isExtra ? btn.extraActions[_xt].midiNumber : btn.midiNumber;
          pedal.modeSelectVarIdx = curCC;
          pedal.modeSelectLevel = 4;
          displayModeSelect(btn.name, pedal.modeSelectCatIdx, 4, curCC, newIdx);

        } else if(pedal.modeSelectLevel == 4) {
          // ── Level 4: primary value confirmed → set midiNumber ────────────
          uint8_t fi = pedal.modeSelectFinalIdx;
          uint8_t newMidi = pedal.modeSelectVarIdx;
          int8_t _xt = pedal.modeSelectExtraActionType;
          bool isExtra = (pedal.modeSelectFromActionSelect && _xt >= 0);
          if(isExtra) {
            btn.extraActions[_xt].midiNumber = newMidi;
          } else {
            applyModeIndex(btn, fi, &_modeSelectMod(pedal));
            btn.midiNumber = newMidi;
          }
          markStateDirty();
          if(modeNeedsVelocity(fi)) {
            uint8_t curVel = (isExtra && _xt >= 0)
                                 ? btn.extraActions[(uint8_t)_xt].velocity
                                 : btn.velocity;
            pedal.modeSelectVelocity = curVel;
            pedal.modeSelectLevel = 5;
            displayModeSelect(btn.name, pedal.modeSelectCatIdx, 5, pedal.modeSelectVelocity, fi);
          } else if(!isExtra && pedal.modeSelectFSIdx != FS_LOAD_ACTION_IDX && modeNeedsRampEntry(fi)) {
            // Mod-switch mode: enter Up Speed submenu
            pedal.modeSelectRampUp = btn.rampUpMs;
            pedal.modeSelectRampDown = btn.rampDownMs;
            pedal.modeSelectLevel = 6;
            displayModeSelect(btn.name, pedal.modeSelectCatIdx, 6, rampRawToSpeedIdx(pedal.modeSelectRampUp), 0);
          } else {
            pedal.inModeSelect = false;
            if(isExtra) {
              pedal.modeSelectFromActionSelect = false;
              pedal.inActionSelect = true;
              displayActionSelect(btn, pedal.actionSelectSlot);
            } else if(pedal.modeSelectFSIdx == FS_LOAD_ACTION_IDX) {
              _saveLoadAction(pedal);
              pedal.menuState = MenuState::ROOT;
              pedal.menuItemIdx = MENU_PRESET_ACTION;
              displayMenuRoot(pedal);
            } else {
              displayModeChange(btn);
            }
          }

        } else if(pedal.modeSelectLevel == 5) {
          // ── Level 5: velocity confirmed → done ───────────────────────────
          int8_t _xt5 = pedal.modeSelectExtraActionType;
          bool isExtra5 = (pedal.modeSelectFromActionSelect && _xt5 >= 0);
          if(isExtra5)
            btn.extraActions[(uint8_t)_xt5].velocity = pedal.modeSelectVelocity;
          else
            btn.velocity = pedal.modeSelectVelocity;
          markStateDirty();
          pedal.inModeSelect = false;
          if(isExtra5) {
            pedal.modeSelectFromActionSelect = false;
            pedal.inActionSelect = true;
            displayActionSelect(btn, pedal.actionSelectSlot);
          } else if(pedal.modeSelectFSIdx == FS_LOAD_ACTION_IDX) {
            _saveLoadAction(pedal);
            pedal.menuState = MenuState::ROOT;
            pedal.menuItemIdx = MENU_PRESET_ACTION;
            displayMenuRoot(pedal);
          } else {
            displayModeChange(btn);
          }

        } else if(pedal.modeSelectLevel == 6) {
          // ── Level 6: Up Speed confirmed → default Down = Up, enter Down ─
          pedal.modeSelectRampDown = pedal.modeSelectRampUp;
          pedal.modeSelectLevel = 7;
          displayModeSelect(btn.name, pedal.modeSelectCatIdx, 7, rampRawToSpeedIdx(pedal.modeSelectRampDown), rampRawToSpeedIdx(pedal.modeSelectRampUp));

        } else {
          // ── Level 7: Down Speed confirmed → apply both and exit ───────────
          btn.rampUpMs = pedal.modeSelectRampUp;
          btn.rampDownMs = pedal.modeSelectRampDown;
          markStateDirty();
          pedal.inModeSelect = false;
          displayModeChange(btn);
        }

      } else {
        int8_t activeIdx = pedal.getActiveButtonIndex();
        if(activeIdx >= 0) {
          // ── Enter action select ────────────────────────────────────────────
          if(pedal.settingsLocked) {
            displayLockedMessage("encBtn");
            return;
          }
          FSButton &btn = pedal.buttons[activeIdx];
          pedal.inActionSelect = true;
          pedal.actionSelectSlot = 0;
          pedal.modeSelectFSIdx = activeIdx;
          pedal.modeSelectFromActionSelect = false;
          displayActionSelect(btn, 0);
        } else if(encBtnDidPresetNav) {
          // ── Released after preset scroll → show home screen ───────────────
          encBtnDidPresetNav = false;
          displayHomeScreen(pedal);
        } else if(!pedal.settingsLocked) {
          // ── Enter main menu ────────────────────────────────────────────────
          pedal.menuState = MenuState::ROOT;
          pedal.menuItemIdx = 0;
          displayMenuRoot(pedal);
        }
      }
    }
    return;
  }

  // ── Press start ────────────────────────────────────────────────────────────
  if(reading == LOW && !encBtnPressing) {
    if((now - encBtn.lastDebounce) < DEBOUNCE_DELAY)
      return;
    encBtnPressing = true;
    encBtnHandled = false;
    encBtnDidPresetNav = false;
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
      if(held >= GLOBAL_TIMEOUT_MS) {
        pedal.settingsLocked = false;
        encBtnHandled = true;
        displayLockChange(false);
      } else {
        static unsigned long lastProgressUpdate = 0;
        if(now - lastProgressUpdate > 80) {
          lastProgressUpdate = now;
          displayUnlockProgress((uint8_t)(held * 100UL / GLOBAL_TIMEOUT_MS));
        }
      }
      return;
    }

    if(pedal.menuState == MenuState::ROUTING && (now - encBtnPressStart) >= GLOBAL_TIMEOUT_MS) {
      handleMenuLongPress(pedal);
      encBtnHandled = true;
      return;
    }

    if(!pedal.inChannelSelect && !pedal.inModeSelect && !pedal.inActionSelect && !pedal.inFSEdit && pedal.menuState == MenuState::NONE && activeIdx < 0 && !pedal.settingsLocked && (now - encBtnPressStart) >= GLOBAL_TIMEOUT_MS) {
      saveCurrentPreset(pedal);
      onPresetSave();
      encBtnHandled = true;
      return;
    }

    if(!pedal.inChannelSelect && !pedal.inModeSelect && !pedal.inActionSelect && pedal.menuState == MenuState::NONE && activeIdx >= 0 && !pedal.buttons[activeIdx].isKeyboard && (now - encBtnPressStart) >= GLOBAL_TIMEOUT_MS) {
      pedal.inChannelSelect = true;
      pedal.channelSelectIdx = activeIdx;
      encBtnHandled = true;
      displayFSChannel(pedal.buttons[activeIdx]);
    }
  }
}
