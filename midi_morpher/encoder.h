#ifndef ENCODER_H
#define ENCODER_H
#include "config.h"
#include "settingsState.h"
#include "controlsState.h"

using EncoderDisplayCallback = void (*)(String, bool, uint8_t);

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
    // pinMode(encBtn.pin, INPUT_PULLUP);
    // pinMode(encBtn.ledPin, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);
}

inline void handleEncoder(MIDIMorpherSettingsState &settings, ControlsState &controlsState, EncoderDisplayCallback displayCallback)
{
    // Check encoder position
    if (encoderPos == lastEncoderPos)
        return;

    int delta = encoderPos - lastEncoderPos;
    lastEncoderPos = encoderPos;

    uint8_t outValue = 0;
    bool isPC = false;
    String fsName = "";

    if (controlsState.fs1Pressed)
    {
        outValue = constrain(settings.fs1Value + delta, 0, 127);
        isPC = settings.fs1IsPC;
        fsName = "FS 1";
        settings.fs1Value = outValue;
    }

    else if (controlsState.fs2Pressed)
    {
        outValue = constrain(settings.fs2Value + delta, 0, 127);
        isPC = settings.fs2IsPC;
        fsName = "FS 2";
        settings.fs2Value = outValue;
    }

    else if (controlsState.hotswitchPressed)
    {
        outValue = constrain(settings.hotswitchValue + delta, 0, 127);
        isPC = settings.hotswitchIsPC;
        fsName = "HotSwitch";
        settings.hotswitchValue = outValue;
    }

    else if (controlsState.extfs1Pressed)
    {
        outValue = constrain(settings.extfs1Value + delta, 0, 127);
        isPC = settings.extfs1IsPC;
        fsName = "ExtFS 1";
        settings.extfs1Value = outValue;
    }

    else if (controlsState.extfs2Pressed)
    {
        outValue = constrain(settings.extfs2Value + delta, 0, 127);
        isPC = settings.extfs2IsPC;
        fsName = "ExtFS 2";
        settings.extfs2Value = outValue;
    }
    else
    {
        // No button held, ignore turn
        outValue = constrain(settings.midiChannel + delta, 0, 15);
        isPC = false;
        fsName = "MIDI Channel";
        settings.midiChannel = outValue;
    }

    displayCallback(fsName, isPC, outValue);
}

#endif
