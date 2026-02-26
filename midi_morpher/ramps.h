#pragma once
#include "config.h"
#include "midiOut.h"

struct MidiCCRamp
{

    enum State
    {
        IDLE,
        RAMPING
    };

    // Config
    uint8_t ccNumber;
    uint8_t midiChannel;
    unsigned long rampTimeMs;

    // Mode control (dynamic)
    bool latchEnabled = false;

    // Runtime
    bool physicalPressed = false;
    bool latchedState = false; // only meaningful if latchEnabled
    uint8_t currentValue = 0;
    uint8_t targetValue = 0;

    State state = IDLE;

    unsigned long rampUpTimeMs;
    unsigned long rampDownTimeMs;

    unsigned long lastStepTime = 0;
    unsigned long stepIntervalUp = 1;
    unsigned long stepIntervalDown = 1;

    unsigned long rampStartTime = 0;
    uint8_t rampStartValue = 0;
    unsigned long currentRampDuration = 0;

    MidiCCRamp(uint8_t cc, uint8_t ch,
               unsigned long upTime,
               unsigned long downTime)
    {
        ccNumber = cc;
        midiChannel = ch;

        rampUpTimeMs = upTime;
        rampDownTimeMs = downTime;

        stepIntervalUp = rampUpTimeMs / 127;
        if (stepIntervalUp == 0)
            stepIntervalUp = 1;

        stepIntervalDown = rampDownTimeMs / 127;
        if (stepIntervalDown == 0)
            stepIntervalDown = 1;
    }

    // ===== Toggle latch dynamically =====
    void setLatch(bool enabled)
    {
        latchEnabled = enabled;
        computeTarget();
    }

    void setCCNumber(uint8_t ccNumber)
    {
        ccNumber = ccNumber;
    }

    void setMidiChannel(uint8_t midiChannel)
    {
        midiChannel = midiChannel;
    }

    void recalcIntervals()
    {

        stepIntervalUp = rampUpTimeMs / 127;
        if (stepIntervalUp == 0)
            stepIntervalUp = 1;

        stepIntervalDown = rampDownTimeMs / 127;
        if (stepIntervalDown == 0)
            stepIntervalDown = 1;
    }

    void setRampTimeUp(unsigned long upTime)
    {
        rampUpTimeMs = upTime;
        recalcIntervals();
    }

    void setRampTimeDown(unsigned long downTime)
    {
        rampDownTimeMs = downTime;
        recalcIntervals();
    }

    // ===== Footswitch events =====
    void press()
    {
        physicalPressed = true;

        if (latchEnabled)
        {
            latchedState = !latchedState;
        }

        computeTarget();
    }

    void release()
    {
        physicalPressed = false;

        if (!latchEnabled)
        {
            computeTarget();
        }
    }

    // ===== Core logic =====
    void computeTarget()
    {

        uint8_t newTarget;

        if (latchEnabled)
        {
            newTarget = latchedState ? 127 : 0;
        }
        else
        {
            bool inverted = (currentValue == 127 && !physicalPressed);
            if (!inverted)
                newTarget = physicalPressed ? 127 : 0;
            else
                newTarget = physicalPressed ? 0 : 127;
        }

        targetValue = newTarget;

        rampStartTime = millis();
        rampStartValue = currentValue;

        currentRampDuration =
            (targetValue > currentValue) ? rampUpTimeMs : rampDownTimeMs;

        state = RAMPING;
    }

    void update()
    {

        if (state == IDLE)
            return;

        if (currentRampDuration <= 1)
        {
            currentValue = targetValue;
            sendMidiCCRamp();
            state = IDLE;
            return;
        }

        unsigned long now = millis();
        unsigned long elapsed = now - rampStartTime;

        if (elapsed >= currentRampDuration)
        {
            currentValue = targetValue;
            sendMidiCCRamp();
            state = IDLE;
            return;
        }

        float progress = (float)elapsed / currentRampDuration;

        int delta = targetValue - rampStartValue;
        uint8_t newValue = rampStartValue + (delta * progress);

        if (newValue != currentValue)
        {
            currentValue = newValue;
            sendMidiCCRamp();
        }
    }

    void sendMidiCCRamp()
    {
        sendMIDI(midiChannel, false, ccNumber, currentValue);
    }
};