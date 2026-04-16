#pragma once
#include "../config.h"
#include "../pedalState.h"
#include "../footswitches/footswitchObject.h"
#include "../statePersistance.h"

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
                         void (*displayFSChannel)(FSButton &)) {
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
  } else {
    uint8_t ch = (uint8_t)constrain((int)pedal.midiChannel + delta, 0, 15);
    pedal.setMidiChannel(ch);
    displayChannelCallback(ch);
  }

  markStateDirty();
}
