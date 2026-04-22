#pragma once
#include "../config.h"
#include "../clock/midiClock.h"
#include "../pedalState.h"
#include "../footswitches/footswitchObject.h"
#include "../statePersistance.h"
#include "../menu/mainMenu.h"

// Defined in encoderButton.h — used here to distinguish encoder-button+rotate
// (MIDI channel) from bare rotate (tempo).
extern bool encBtnPressing;

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
                         void (*displayModeSelect)(const char *, uint8_t, uint8_t, uint8_t, uint8_t)) {
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

  // ── Three-level mode select (category / sub-group / variant / CC Hi+Lo) ────
  if(pedal.inModeSelect) {
    const ModeCategory &cat = modeCategories[pedal.modeSelectCatIdx];
    const char *fsName = pedal.buttons[pedal.modeSelectFSIdx].name;
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
        // CC Hi value (0-127, acceleration applies)
        int next = constrain((int)pedal.modeSelectVarIdx + delta * accelMult, 0, 127);
        pedal.modeSelectVarIdx = (uint8_t)next;
        displayModeSelect(fsName, pedal.modeSelectCatIdx, 2,
                          pedal.modeSelectVarIdx, 0);
      }
    } else {
      // Level 3: CC Lo value (0-127, acceleration applies)
      int next = constrain((int)pedal.modeSelectSubVarIdx + delta * accelMult, 0, 127);
      pedal.modeSelectSubVarIdx = (uint8_t)next;
      displayModeSelect(fsName, pedal.modeSelectCatIdx, 3,
                        pedal.modeSelectVarIdx, pedal.modeSelectSubVarIdx);
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
    if(btn.isKeyboard) {
      // Keyboard mode: cycle through available keys (no acceleration, small range)
      int next = constrain((int)btn.midiNumber + delta, 0, (int)(NUM_HID_KEYS - 1));
      btn.midiNumber = (uint8_t)next;
      displayFSCallback(btn);
      markStateDirty();
      return;
    }
    if(btn.isModSwitch) {
      // Modulation modes: range extends one step below CC0 to PB_SENTINEL (PB).
      // Cursor space: -1 = PB, 0..127 = CC0..CC127. Acceleration applies.
      int cur  = (btn.midiNumber == PB_SENTINEL) ? -1 : (int)btn.midiNumber;
      int next = constrain(cur + delta * accelMult, -1, 127);
      btn.midiNumber = (next < 0) ? PB_SENTINEL : (uint8_t)next;
    } else {
      uint8_t maxVal;
      int     step;
      if(btn.isScene) {
        maxVal = btn.modMode.sceneMaxVal;
        step   = delta;
      } else if(btn.isSystem) {
        maxVal = NUM_SYS_CMDS - 1;
        step   = delta;
      } else {
        maxVal = 127;
        step   = delta * accelMult;
      }
      btn.midiNumber = (uint8_t)constrain((int)btn.midiNumber + step, 0, (int)maxVal);
    }
    displayFSCallback(btn);
  } else if(encBtnPressing) {
    // Encoder button held + rotate → global MIDI channel (no acceleration).
    uint8_t ch = (uint8_t)constrain((int)pedal.midiChannel + delta, 0, 15);
    pedal.setMidiChannel(ch);
    displayChannelCallback(ch);
  } else {
    // Bare rotate → adjust tempo. Acceleration applies (range 20–300).
    float newBpm = midiClock.bpm + (float)(delta * accelMult);
    midiClock.setBpm(newBpm);
    midiClock.externalSync = false;   // manual override breaks ext sync
    displayBpmCallback(midiClock.bpm);
  }

  markStateDirty();
}
