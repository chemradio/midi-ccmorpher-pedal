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
                         void (*displayModeSelect)(const char *, uint8_t, uint8_t, bool)) {
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

  // ── Two-level mode select (category / variant) ─────────────────────────────
  if(pedal.inModeSelect) {
    if(pedal.modeSelectLevel == 0) {
      int next = constrain((int)pedal.modeSelectCatIdx + delta, 0, NUM_CATEGORIES - 1);
      pedal.modeSelectCatIdx = (uint8_t)next;
      displayModeSelect(pedal.buttons[pedal.modeSelectFSIdx].name,
                        pedal.modeSelectCatIdx, 0, false);
    } else {
      const ModeCategory &cat = modeCategories[pedal.modeSelectCatIdx];
      int next = constrain((int)pedal.modeSelectVarIdx + delta, 0, (int)cat.count - 1);
      pedal.modeSelectVarIdx = (uint8_t)next;
      displayModeSelect(pedal.buttons[pedal.modeSelectFSIdx].name,
                        pedal.modeSelectCatIdx, pedal.modeSelectVarIdx, true);
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
