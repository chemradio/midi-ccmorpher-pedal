#pragma once
#include "../config.h"
#include "../clock/midiClock.h"
#include "../pedalState.h"
#include "../footswitches/footswitchObject.h"
#include "../statePersistance.h"
#include "../menu/mainMenu.h"

// Defined in encoderButton.h — used here to distinguish encoder-button+rotate
// (preset scroll) from bare rotate (tempo).
extern bool encBtnPressing;
extern bool encBtnDidPresetNav;

// Declare global variables as extern
extern volatile int encoderPos;
extern int lastEncoderPos;
extern volatile uint8_t encoderLastEncoded;
extern volatile int8_t encoderStepAccumulator;

// Remove inline from ISR
void IRAM_ATTR encoderISR();

inline void initEncoder()
{
    pinMode(ENC_A, INPUT_PULLUP);
    pinMode(ENC_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);
}

inline void handleEncoder(PedalState &pedal,
                         void (*displayFSCallback)(FSButton &),
                         void (*displayChannelCallback)(uint8_t),
                         void (*displayLockedMessage)(String),
                         void (*displayFSChannel)(FSButton &),
                         void (*displayBpmCallback)(float),
                         void (*displayModeSelect)(const char *, uint8_t, uint8_t, uint8_t, uint8_t),
                         void (*displayActionSelectFn)(FSButton &, uint8_t)) {
  if(encoderPos == lastEncoderPos) return;

  int delta      = encoderPos - lastEncoderPos;
  lastEncoderPos = encoderPos;

  // ── Acceleration ────────────────────────────────────────────────────────────
  // Multiply delta by a factor based on time since last tick. Only applied to
  // wide-range fields (midiNumber for CC/PC/Note — 0–127). Scene modes, channel
  // select, and per-FS channel have tiny ranges and would feel jumpy.
  static unsigned long _lastEncTime = 0;
  unsigned long now = millis();
  unsigned long dt  = now - _lastEncTime;
  _lastEncTime = now;
  int accelMult = 1;
  if(dt < 15)      accelMult = 8;
  else if(dt < 30) accelMult = 4;
  else if(dt < 60) accelMult = 2;

  // ── Main menu ──────────────────────────────────────────────────────────────
  if(pedal.menuState != MenuState::NONE) {
    handleMenuRotate(pedal, delta);
    return;
  }

  // ── Action selector slot scroll ────────────────────────────────────────────
  if(pedal.inActionSelect) {
    FSButton &btn = pedal.buttons[pedal.modeSelectFSIdx];
    bool expanded = btn.extraActions[0].enabled || btn.extraActions[1].enabled || btn.extraActions[2].enabled;
    uint8_t slotCount = expanded ? 5 : 2;
    int next = constrain((int)pedal.actionSelectSlot + delta, 0, (int)slotCount - 1);
    pedal.actionSelectSlot = (uint8_t)next;
    displayActionSelectFn(btn, pedal.actionSelectSlot);
    return;
  }

  // ── FS cursor edit menu ────────────────────────────────────────────────────
  if(pedal.inFSEdit) {
    FSButton &btn = pedal.fsEditTarget();
    FSEditRow rows[MAX_FS_EDIT_ROWS];
    uint8_t rowCount = buildFSEditRows(btn, rows);
    if(pedal.fsEditCursor >= rowCount) pedal.fsEditCursor = rowCount - 1;

    if(!pedal.fsEditEditing) {
      int next = constrain((int)pedal.fsEditCursor + delta, 0, (int)rowCount - 1);
      pedal.fsEditCursor = (uint8_t)next;
    } else {
      FSEditRow row = rows[pedal.fsEditCursor];
      uint8_t mi = btn.modeIndex;
      switch(row) {
        case FSEditRow::CATEGORY: {
          int ci  = (int)categoryForModeIndex(mi);
          int nci = constrain(ci + delta, 0, (int)NUM_CATEGORIES - 1);
          if(nci != ci) {
            uint8_t newMi = modeCategories[(uint8_t)nci].firstIdx;
            pedal.fsEditApplyMode(newMi);
            _syncFSEditTarget(pedal);
            markStateDirty();
            // Clamp cursor after row rebuild
            FSEditRow nr[MAX_FS_EDIT_ROWS]; uint8_t nc = buildFSEditRows(pedal.fsEditTarget(), nr);
            if(pedal.fsEditCursor >= nc) pedal.fsEditCursor = 0;
          }
          break;
        }
        case FSEditRow::CC_TRIGGER: {
          uint8_t v = (mi >= 2 && mi <= 4) ? (mi - 2) : 0;
          int nv = constrain((int)v + delta, 0, 2);
          if(nv != (int)v) {
            pedal.fsEditApplyMode((uint8_t)(2 + nv));
            _syncFSEditTarget(pedal);
            markStateDirty();
            FSEditRow nr[MAX_FS_EDIT_ROWS]; uint8_t nc = buildFSEditRows(pedal.fsEditTarget(), nr);
            if(pedal.fsEditCursor >= nc) pedal.fsEditCursor = 0;
          }
          break;
        }
        case FSEditRow::RAMP_SHAPE: {
          if(mi >= 6 && mi <= 17) {
            uint8_t sh=(mi-6)/4, di=((mi-6)/2)%2, tr=(mi-6)%2;
            int ns = constrain((int)sh + delta, 0, 2);
            pedal.fsEditApplyMode((uint8_t)(6 + ns*4 + di*2 + tr));
            _syncFSEditTarget(pedal); markStateDirty();
          }
          break;
        }
        case FSEditRow::MOD_DIR: {
          uint8_t newMi = mi;
          if(mi >= 6  && mi <= 17) { uint8_t sh=(mi-6)/4,di=((mi-6)/2)%2,tr=(mi-6)%2; newMi=6+sh*4+(1-di)*2+tr; }
          else if(mi >= 24 && mi <= 27) { uint8_t di=((mi-24)/2)%2,tr=(mi-24)%2; newMi=24+(1-di)*2+tr; }
          else if(mi >= 28 && mi <= 31) { uint8_t di=((mi-28)/2)%2,tr=(mi-28)%2; newMi=28+(1-di)*2+tr; }
          if(newMi != mi) { pedal.fsEditApplyMode(newMi); _syncFSEditTarget(pedal); markStateDirty(); }
          break;
        }
        case FSEditRow::MOD_TRIG: {
          uint8_t newMi = mi;
          if(mi >= 6  && mi <= 17) { uint8_t sh=(mi-6)/4,di=((mi-6)/2)%2,tr=(mi-6)%2; newMi=6+sh*4+di*2+(1-tr); }
          else if(mi >= 18 && mi <= 23) { uint8_t wa=(mi-18)/2,tr=(mi-18)%2; newMi=18+wa*2+(1-tr); }
          else if(mi >= 24 && mi <= 27) { uint8_t di=((mi-24)/2)%2,tr=(mi-24)%2; newMi=24+di*2+(1-tr); }
          else if(mi >= 28 && mi <= 31) { uint8_t di=((mi-28)/2)%2,tr=(mi-28)%2; newMi=28+di*2+(1-tr); }
          if(newMi != mi) { pedal.fsEditApplyMode(newMi); _syncFSEditTarget(pedal); markStateDirty(); }
          break;
        }
        case FSEditRow::LFO_WAVE: {
          if(mi >= 18 && mi <= 23) {
            uint8_t wa=(mi-18)/2, tr=(mi-18)%2;
            int nw = constrain((int)wa + delta, 0, 2);
            pedal.fsEditApplyMode((uint8_t)(18 + nw*2 + tr));
            _syncFSEditTarget(pedal); markStateDirty();
          }
          break;
        }
        case FSEditRow::SCENE_UNIT: {
          if(mi >= 32 && mi <= 39) {
            uint8_t un=(mi-32)/2, sc=(mi-32)%2;
            int nu = constrain((int)un + delta, 0, 3);
            pedal.fsEditApplyMode((uint8_t)(32 + nu*2 + sc));
            _syncFSEditTarget(pedal); markStateDirty();
          }
          break;
        }
        case FSEditRow::SCENE_STYLE: {
          if(mi >= 32 && mi <= 39) {
            uint8_t un=(mi-32)/2, sc=(mi-32)%2;
            pedal.fsEditApplyMode((uint8_t)(32 + un*2 + (1-sc)));
            _syncFSEditTarget(pedal); markStateDirty();
          }
          break;
        }
        case FSEditRow::PRESET_DIR: {
          if(mi >= 44 && mi <= 46) {
            int nd = constrain((int)(mi-44) + delta, 0, 2);
            pedal.fsEditApplyMode((uint8_t)(44 + nd));
            _syncFSEditTarget(pedal); markStateDirty();
            FSEditRow nr[MAX_FS_EDIT_ROWS]; uint8_t nc = buildFSEditRows(pedal.fsEditTarget(), nr);
            if(pedal.fsEditCursor >= nc) pedal.fsEditCursor = 0;
          }
          break;
        }
        case FSEditRow::NUMBER: {
          if(btn.isModSwitch) {
            int cur = (btn.midiNumber == PB_SENTINEL) ? -1 : (int)btn.midiNumber;
            int nxt = constrain(cur + delta * accelMult, -1, 127);
            btn.midiNumber = (nxt < 0) ? PB_SENTINEL : (uint8_t)nxt;
          } else if(btn.isNote || btn.isPC) {
            btn.midiNumber = (uint8_t)constrain((int)btn.midiNumber + delta * accelMult, 0, 127);
          } else if(btn.isScene) {
            btn.midiNumber = (uint8_t)constrain((int)btn.midiNumber + delta, 0, (int)btn.modMode.sceneMaxVal);
          } else if(btn.isSystem) {
            btn.midiNumber = (uint8_t)constrain((int)btn.midiNumber + delta, 0, NUM_SYS_CMDS - 1);
          } else if(btn.isKeyboard) {
            btn.midiNumber = (uint8_t)constrain((int)btn.midiNumber + delta, 0, (int)(NUM_HID_KEYS - 1));
          } else if(btn.mode == FootswitchMode::PresetNum) {
            uint8_t maxP = pedal.globalSettings.presetCount > 0 ? pedal.globalSettings.presetCount - 1 : 0;
            btn.midiNumber = (uint8_t)constrain((int)btn.midiNumber + delta, 0, (int)maxP);
          } else if(btn.mode == FootswitchMode::Multi) {
            int step = (delta > 0) ? 1 : -1;
            int nxt  = (int)btn.midiNumber + step;
            while(nxt >= 0 && nxt < (int)MAX_MULTI_SCENES && multiScenes[nxt].name[0] == '\0') nxt += step;
            if(nxt >= 0 && nxt < (int)MAX_MULTI_SCENES && multiScenes[nxt].name[0] != '\0')
              btn.midiNumber = (uint8_t)nxt;
          } else {
            btn.midiNumber = (uint8_t)constrain((int)btn.midiNumber + delta * accelMult, 0, 127);
          }
          _syncFSEditTarget(pedal); markStateDirty();
          break;
        }
        case FSEditRow::CC_HI:
          btn.ccHigh = (uint8_t)constrain((int)btn.ccHigh + delta * accelMult, 0, 127);
          _syncFSEditTarget(pedal); markStateDirty();
          break;
        case FSEditRow::CC_LO:
          btn.ccLow = (uint8_t)constrain((int)btn.ccLow + delta * accelMult, 0, 127);
          _syncFSEditTarget(pedal); markStateDirty();
          break;
        case FSEditRow::CC_VAL:
          btn.ccHigh = (uint8_t)constrain((int)btn.ccHigh + delta * accelMult, 0, 127);
          _syncFSEditTarget(pedal); markStateDirty();
          break;
        case FSEditRow::VELOCITY:
          btn.velocity = (uint8_t)constrain((int)btn.velocity + delta * accelMult, 1, 127);
          _syncFSEditTarget(pedal); markStateDirty();
          break;
        case FSEditRow::KB_MOD: {
          uint8_t cur = (btn.fsChannel == 0xFF) ? 0 : btn.fsChannel;
          btn.fsChannel = (uint8_t)(((int)cur + delta + 16) % 16);
          _syncFSEditTarget(pedal); markStateDirty();
          break;
        }
        case FSEditRow::RAMP_UP: {
          uint8_t cur = rampRawToSpeedIdx(btn.rampUpMs);
          btn.rampUpMs = speedIdxToRampRaw((uint8_t)constrain((int)cur + delta, 0, RAMP_SPEED_TABLE_SIZE - 1));
          _syncFSEditTarget(pedal); markStateDirty();
          break;
        }
        case FSEditRow::RAMP_DN: {
          uint8_t cur = rampRawToSpeedIdx(btn.rampDownMs);
          btn.rampDownMs = speedIdxToRampRaw((uint8_t)constrain((int)cur + delta, 0, RAMP_SPEED_TABLE_SIZE - 1));
          _syncFSEditTarget(pedal); markStateDirty();
          break;
        }
        case FSEditRow::CHANNEL: {
          int cur = (btn.fsChannel == 0xFF) ? 0 : (int)(btn.fsChannel + 1);
          int nxt = constrain(cur + delta, 0, 16);
          btn.fsChannel = (nxt == 0) ? 0xFF : (uint8_t)(nxt - 1);
          _syncFSEditTarget(pedal); markStateDirty();
          break;
        }
        default: break;
      }
    }
    displayFSEditMenu(pedal);
    return;
  }

  // ── Three-level mode select (category / sub-group / variant / CC Hi+Lo) ────
  if(pedal.inModeSelect) {
    const ModeCategory &cat = modeCategories[pedal.modeSelectCatIdx];
    const char *fsName = pedal.modeSelectBtn().name;
    if(pedal.modeSelectLevel == 0) {
      int next = constrain((int)pedal.modeSelectCatIdx + delta, 0, NUM_CATEGORIES - 1);
      pedal.modeSelectCatIdx = (uint8_t)next;
      displayModeSelect(fsName, pedal.modeSelectCatIdx, 0, 0, 0);
    } else if(pedal.modeSelectLevel == 1) {
      bool isMultiCat = (cat.firstIdx < NUM_MODES &&
                         modes[cat.firstIdx].mode == FootswitchMode::Multi);
      if(isMultiCat) {
        int step = (delta > 0) ? 1 : -1;
        int next = (int)pedal.modeSelectVarIdx + step;
        while(next >= 0 && next < (int)MAX_MULTI_SCENES &&
              multiScenes[next].name[0] == '\0') next += step;
        if(next >= 0 && next < (int)MAX_MULTI_SCENES &&
           multiScenes[next].name[0] != '\0')
          pedal.modeSelectVarIdx = (uint8_t)next;
        displayModeSelect(fsName, pedal.modeSelectCatIdx, 1,
                          pedal.modeSelectVarIdx, 0);
        return;
      }
      int maxIdx = (cat.subGroupCount > 0)
                     ? (int)cat.subGroupCount - 1
                     : (int)cat.count - 1;
      int next = constrain((int)pedal.modeSelectVarIdx + delta, 0, maxIdx);
      pedal.modeSelectVarIdx = (uint8_t)next;
      displayModeSelect(fsName, pedal.modeSelectCatIdx, 1,
                        pedal.modeSelectVarIdx, 0);
    } else if(pedal.modeSelectLevel == 2) {
      if(cat.subGroupCount > 0) {
        int next = constrain((int)pedal.modeSelectSubVarIdx + delta,
                             0, (int)cat.subGroupSize - 1);
        pedal.modeSelectSubVarIdx = (uint8_t)next;
        displayModeSelect(fsName, pedal.modeSelectCatIdx, 2,
                          pedal.modeSelectVarIdx, pedal.modeSelectSubVarIdx);
      } else {
        // CC Hi/Single value (0-127, acceleration applies)
        int next = constrain((int)pedal.modeSelectVarIdx + delta * accelMult, 0, 127);
        pedal.modeSelectVarIdx = (uint8_t)next;
        displayModeSelect(fsName, pedal.modeSelectCatIdx, 2,
                          pedal.modeSelectVarIdx, pedal.modeSelectCCVariant);
      }
    } else if(pedal.modeSelectLevel == 3) {
      // Level 3: CC Lo value (0-127, acceleration applies)
      int next = constrain((int)pedal.modeSelectSubVarIdx + delta * accelMult, 0, 127);
      pedal.modeSelectSubVarIdx = (uint8_t)next;
      displayModeSelect(fsName, pedal.modeSelectCatIdx, 3,
                        pedal.modeSelectVarIdx, pedal.modeSelectSubVarIdx);
    } else if(pedal.modeSelectLevel == 4) {
      // Level 4: primary value (midiNumber) — range depends on final mode
      uint8_t fi = pedal.modeSelectFinalIdx;
      if(fi < NUM_MODES && modes[fi].isModSwitch) {
        int cur  = (pedal.modeSelectVarIdx == PB_SENTINEL) ? -1 : (int)pedal.modeSelectVarIdx;
        int next = constrain(cur + delta * accelMult, -1, 127);
        pedal.modeSelectVarIdx = (next < 0) ? PB_SENTINEL : (uint8_t)next;
      } else if(fi < NUM_MODES && modes[fi].isSystem) {
        int next = constrain((int)pedal.modeSelectVarIdx + delta, 0, NUM_SYS_CMDS - 1);
        pedal.modeSelectVarIdx = (uint8_t)next;
      } else if(fi < NUM_MODES && modes[fi].isKeyboard) {
        int next = constrain((int)pedal.modeSelectVarIdx + delta, 0, (int)(NUM_HID_KEYS - 1));
        pedal.modeSelectVarIdx = (uint8_t)next;
      } else if(fi < NUM_MODES && modes[fi].isScene) {
        int next = constrain((int)pedal.modeSelectVarIdx + delta, 0, (int)modes[fi].sceneMaxVal);
        pedal.modeSelectVarIdx = (uint8_t)next;
      } else if(fi < NUM_MODES && modes[fi].mode == FootswitchMode::PresetNum) {
        uint8_t maxP = pedal.globalSettings.presetCount > 0 ? pedal.globalSettings.presetCount - 1 : 0;
        int next = constrain((int)pedal.modeSelectVarIdx + delta, 0, (int)maxP);
        pedal.modeSelectVarIdx = (uint8_t)next;
      } else {
        int next = constrain((int)pedal.modeSelectVarIdx + delta * accelMult, 0, 127);
        pedal.modeSelectVarIdx = (uint8_t)next;
      }
      displayModeSelect(fsName, pedal.modeSelectCatIdx, 4,
                        pedal.modeSelectVarIdx, pedal.modeSelectFinalIdx);
    } else if(pedal.modeSelectLevel == 5) {
      // Level 5: velocity (1-127, acceleration applies)
      int next = constrain((int)pedal.modeSelectVelocity + delta * accelMult, 1, 127);
      pedal.modeSelectVelocity = (uint8_t)next;
      displayModeSelect(fsName, pedal.modeSelectCatIdx, 5,
                        pedal.modeSelectVelocity, pedal.modeSelectFinalIdx);
    } else if(pedal.modeSelectLevel == 6) {
      // Level 6: Up Speed — scroll through 51 ms steps + 17 note-value entries
      uint8_t cur  = rampRawToSpeedIdx(pedal.modeSelectRampUp);
      int     next = constrain((int)cur + delta, 0, (int)RAMP_SPEED_TABLE_SIZE - 1);
      pedal.modeSelectRampUp = speedIdxToRampRaw((uint8_t)next);
      displayModeSelect(fsName, pedal.modeSelectCatIdx, 6, (uint8_t)next, 0);
    } else {
      // Level 7: Down Speed
      uint8_t cur  = rampRawToSpeedIdx(pedal.modeSelectRampDown);
      int     next = constrain((int)cur + delta, 0, (int)RAMP_SPEED_TABLE_SIZE - 1);
      pedal.modeSelectRampDown = speedIdxToRampRaw((uint8_t)next);
      uint8_t upIdx = rampRawToSpeedIdx(pedal.modeSelectRampUp);
      displayModeSelect(fsName, pedal.modeSelectCatIdx, 7, (uint8_t)next, upIdx);
    }
    return;
  }

  // ── Per-FS channel select (entered via long-press of encoder button) ────────
  if(pedal.inChannelSelect) {
    FSButton &btn = pedal.buttons[pedal.channelSelectIdx];
    // Position 0 = GLB (0xFF), positions 1–16 = channels 1–16.
    int current = (btn.fsChannel == 0xFF) ? 0 : (int)(btn.fsChannel + 1);
    int next    = constrain(current + delta, 0, 16);
    btn.fsChannel = (next == 0) ? 0xFF : (uint8_t)(next - 1);
    markStateDirty();
    displayFSChannel(btn);
    return;
  }

  if(pedal.settingsLocked) {
    displayLockedMessage("enc");
    return;
  }

  int8_t activeButtonIndex = pedal.getActiveButtonIndex();

  if(activeButtonIndex >= 0) {
    FSButton &btn = pedal.buttons[activeButtonIndex];
    if(btn.mode == FootswitchMode::Multi) {
      // Multi: scroll through scene slots while FS held
      int step = (delta > 0) ? 1 : -1;
      int next = (int)btn.midiNumber + step;
      while(next >= 0 && next < (int)MAX_MULTI_SCENES &&
            multiScenes[next].name[0] == '\0') next += step;
      if(next >= 0 && next < (int)MAX_MULTI_SCENES &&
         multiScenes[next].name[0] != '\0') {
        btn.midiNumber = (uint8_t)next;
        markStateDirty();
        displayFSCallback(btn);
      }
      return;
    }
    // FS+encTurn reserved for future use — ignore all other modes
    return;
  } else if(encBtnPressing) {
    // Encoder button held + rotate → scroll presets (acceleration for large counts).
    uint8_t count = pedal.globalSettings.presetCount;
    int step = delta * (count > 20 ? accelMult : 1);
    int next = ((int)activePreset + step % (int)count + (int)count) % (int)count;
    presetNavDirect    = (int8_t)next;
    encBtnDidPresetNav = true;
  } else {
    // Bare rotate → adjust tempo. Acceleration applies (range 20–300).
    float newBpm = midiClock.bpm + (float)(delta * accelMult);
    midiClock.setBpm(newBpm);
    midiClock.externalSync = false;   // manual override breaks ext sync
    displayBpmCallback(midiClock.bpm);
  }

  markStateDirty();
}
