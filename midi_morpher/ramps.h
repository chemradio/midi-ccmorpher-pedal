#pragma once
#include "config.h"
#include "midiOut.h"
#include <Adafruit_NeoPixel.h>

enum RampingState
{
    IDLE,
    RAMPING
};

struct MidiCCRamp
{
    bool isLinear = true;
    bool switchPressed = false;
    bool inverted = false;
    bool latching = false;
    bool isLatched = false;
    bool isActivated = false;
    uint8_t targetValue = 0;
    uint8_t currentValue = 0;
    unsigned long rampUpTimeMs = DEFAULT_RAMP_SPEED;
    unsigned long rampDownTimeMs = DEFAULT_RAMP_SPEED;
    unsigned long rampStartTime = 0;
    uint8_t rampStartValue = 0;
    unsigned long currentRampDuration = 0;
    bool isRamping = false;
    uint8_t midiChannel = 0;
    uint8_t midiCCNumber = 25;

    void _setLED(bool state)
    {
        digitalWrite(HS_LED, state);
    }

    void setLatch(bool enabled)
    {
        latching = enabled;
        if (currentValue > 63)
        {
            inverted = true;
        }
        else
        {
            inverted = false;
        }
        _setLED(false);
    }

    void setCurveType(bool isExp)
    {
        isLinear = !isExp;
    }

    void setCCNumber(uint8_t ccNumber)
    {
        midiCCNumber = ccNumber;
    }

    void setMidiChannel(uint8_t channel)
    {
        midiChannel = channel; // Fixed: was assigning to itself
    }

    void setRampTimeUp(unsigned long upTime)
    {
        rampUpTimeMs = upTime;
    }

    void setRampTimeDown(unsigned long downTime)
    {
        rampDownTimeMs = downTime;
    }

    void calcAndStartRamp()
    {
        if (isActivated)
        {
            targetValue = inverted ? 0 : 127;
        }
        else
        {
            targetValue = inverted ? 127 : 0;
        }

        rampStartTime = millis();
        rampStartValue = currentValue;
        uint8_t distance = abs(targetValue - currentValue);
        if (distance == 0)
        {
            isRamping = false;
            return;
        }
        float fraction = distance / 127.0f;
        bool goingUp = (targetValue > currentValue);
        unsigned long fullDuration = goingUp ? rampUpTimeMs : rampDownTimeMs;
        currentRampDuration = fullDuration * fraction;
        isRamping = true;
    }

    void press()
    {
        if (switchPressed)
            return;
        switchPressed = true;

        if (latching)
        {
            isLatched = !isLatched;
            isActivated = !isActivated;
        }
        else
        {
            isActivated = true;
        }

        _setLED(isActivated);
        calcAndStartRamp();
    }

    void release()
    {
        if (!switchPressed)
            return;
        switchPressed = false;

        if (latching)
            return;

        isActivated = false;
        _setLED(isActivated);
        calcAndStartRamp();
    }

    void update()
    {

        if (!isRamping)
            // Serial.println("exit not ramping");
            return;

        if (currentRampDuration <= 1)
        {
            currentValue = targetValue;
            isRamping = false;
            sendMIDI(midiChannel, false, midiCCNumber, currentValue);
            return;
        }

        unsigned long now = millis();
        unsigned long elapsed = now - rampStartTime;

        if (elapsed >= currentRampDuration)
        {
            currentValue = targetValue;
            isRamping = false;
            sendMIDI(midiChannel, false, midiCCNumber, currentValue);
            return;
        }

        float t = (float)elapsed / currentRampDuration;
        if (t > 1.0f)
            t = 1.0f;

        float shaped;

        if (!isLinear)
        {
            shaped = t * t;
            // float inv = 1.0f - t;
            // shaped = 1.0f - (inv * inv * inv);
        }
        else // linear
        {
            shaped = t;
        }

        int delta = targetValue - rampStartValue;
        uint8_t newValue = rampStartValue + (delta * shaped);

        if (newValue != currentValue)
        {
            currentValue = newValue;
            sendMIDI(midiChannel, false, midiCCNumber, currentValue);
        }
    }
};