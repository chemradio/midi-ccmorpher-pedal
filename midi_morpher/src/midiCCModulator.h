#pragma once
#include "config.h"
#include "midiOut.h"
#include <Adafruit_NeoPixel.h>

struct MidiCCModulator
{
    enum ModulationType
    {
        RAMPER,
        LFO,
        STEPPER,
        RANDOM
    };
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
    bool isModulating = false;
    uint8_t midiChannel = 0;
    uint8_t midiCCNumber = 25;
    ModulationType modType = ModulationType::RAMPER;

    float lfoRateHz = 1.0f;               // cycles per second
    unsigned long lfoStartTime = 0;       // set to millis() when press() is called
    bool lfoFinishing = false;            // true when completing final cycle before stopping
    unsigned long randomIntervalMs = 300; // ms between random jumps
    unsigned long lastRandomTime = 0;
    uint8_t randomMin = 0;           // lower bound for random target (0-127)
    uint8_t randomMax = 127;         // upper bound for random target (0-127)
    uint8_t stepSize = 10;           // CC values per step (1-127). Timing is automatic.
    unsigned long lastStepTime = 0;  // timestamp of last step
    bool stepperRampingDown = false; // true when ramping back to 0 on release

    void updateRamper();
    void updateLFO();
    void updateStepper();
    void updateRandomStepper();

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
            isModulating = false;
            return;
        }
        float fraction = distance / 127.0f;
        bool goingUp = (targetValue > currentValue);
        unsigned long fullDuration = goingUp ? rampUpTimeMs : rampDownTimeMs;
        currentRampDuration = fullDuration * fraction;
        isModulating = true;
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

        lfoStartTime = millis();
        lfoFinishing = false;
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

        if (modType == ModulationType::LFO)
        {
            lfoFinishing = true;
            isModulating = true;
        }
        calcAndStartRamp();
    }

    void update()
    {
        if (!isModulating)
            return;

        switch (modType)
        {
        case ModulationType::RAMPER:
            updateRamper();
            return;
        case ModulationType::LFO:
            updateLFO();
            return;
        case ModulationType::STEPPER:
            updateStepper();
            return;
        case ModulationType::RANDOM:
            updateRandomStepper();
            return;
        }
        return;
    }
};

#include "modulators/ramper.h"
#include "modulators/lfo.h"
#include "modulators/stepper.h"
#include "modulators/randomStepper.h"