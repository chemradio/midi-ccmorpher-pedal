#ifndef ENCODER_H
#define ENCODER_H
#include "config.h"
#include "pedalState.h"

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

inline void handleEncoder(PedalState pedal, void (*displayCallback)(String, bool, uint8_t))
{
    // Check encoder position
    if (encoderPos == lastEncoderPos)
        return;

    int delta = encoderPos - lastEncoderPos;
    lastEncoderPos = encoderPos;

    uint8_t outValue = 0;
    bool isPC = false;
    String fsName = "";

    int8_t activeButtonIndex = pedal.getActiveButtonIndex();

    if (activeButtonIndex >= 0)
    {
        pedal.adjustActiveMidiNumber(delta);
        fsName = pedal.buttons[activeButtonIndex].name;
        isPC = pedal.buttons[activeButtonIndex].isPC;
        outValue = pedal.buttons[activeButtonIndex].midiNumber;
    }
    else
    {
        uint8_t midiChannel = pedal.midiChannel;
        outValue = constrain(midiChannel + delta, 0, 15);
        pedal.midiChannel = outValue;
        fsName = "MIDI Channel";
        isPC = false;
    }
    displayCallback(fsName, isPC, outValue);
}

#endif
