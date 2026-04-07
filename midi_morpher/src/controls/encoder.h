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

inline bool btnLastState = HIGH;
inline unsigned long btnLastDebounce = 0;

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
                         void (*displayLockedMessage)(String)) {
  if(encoderPos == lastEncoderPos) return;

  int delta      = encoderPos - lastEncoderPos;
  lastEncoderPos = encoderPos;

  if(pedal.settingsLocked) {
    displayLockedMessage("enc");
    return;
  }

  int8_t activeButtonIndex = pedal.getActiveButtonIndex();

  if(activeButtonIndex >= 0) {
    FSButton &btn = pedal.buttons[activeButtonIndex];
    // Scene modes clamp to their encoder ceiling; all others use full MIDI range.
    uint8_t maxVal = btn.isScene ? btn.modMode.sceneMaxVal : 127;
    btn.midiNumber = (uint8_t)constrain((int)btn.midiNumber + delta, 0, (int)maxVal);
    displayFSCallback(btn);
  } else {
    uint8_t ch = (uint8_t)constrain((int)pedal.midiChannel + delta, 0, 15);
    pedal.setMidiChannel(ch);
    displayChannelCallback(ch);
  }

  markStateDirty();
}
