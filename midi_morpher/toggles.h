#pragma once
#include "config.h"
#include "pedalState.h"

struct Toggle
{
    uint8_t pin;
    const char *name;
    bool lastState;
};

inline Toggle toggles[] = {
    {MS2_PIN, "HotSwitch Latching", HIGH},
    {DIR_PIN, "Ramp Direction", HIGH},
    {LESW_PIN, "Linear/Exp", HIGH},
    {LST_PIN, "Lock Settings", HIGH}};

inline void initToggles()
{
    for (int i = 0; i < 4; i++)
    {
        pinMode(toggles[i].pin, INPUT_PULLUP);
    }
}

inline bool handleToggleChange(Toggle &toggle, PedalState &pedal)
{
    bool state = digitalRead(toggle.pin);
    bool isRampDir = (toggle.pin == DIR_PIN);
    if (state != toggle.lastState)
    {
        toggle.lastState = state;
        if (toggle.pin == MS2_PIN)
        {
            pedal.hotSwitchLatching = !state;
        }
        else if (toggle.pin == DIR_PIN)
        {
            pedal.rampDirectionUp = !state;
        }
        else if (toggle.pin == LESW_PIN)
        {
            pedal.rampLinearCurve = !state;
        }
        else if (toggle.pin == LST_PIN)
        {
            pedal.settingsLocked = !state;
        }
        return true;
    }
    return false;
}
