#ifndef TOGGLES_H
#define TOGGLES_H
#include "config.h"
#include "settingsState.h"
#include "controlsState.h"

using ToggleDisplayCallback = void (*)(String, bool, bool);

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

inline bool handleToggle(Toggle &toggle, MIDIMorpherSettingsState &settings, ControlsState &controlsState, ToggleDisplayCallback displayCallback)
{
    bool state = digitalRead(toggle.pin);
    bool isRampDir = (toggle.pin == DIR_PIN);
    if (state != toggle.lastState)
    {
        toggle.lastState = state;
        if (toggle.pin == MS2_PIN)
        {
            settings.hotSwitchLatching = !state;
        }
        else if (toggle.pin == DIR_PIN)
        {
            settings.rampDirectionUp = !state;
        }
        else if (toggle.pin == LESW_PIN)
        {
            settings.rampLinearCurve = !state;
        }
        else if (toggle.pin == LST_PIN)
        {
            settings.settingsLocked = !state;
        }
        displayCallback(String(toggle.name), !state, isRampDir);
        return true;
    }
    return false;
}

#endif
