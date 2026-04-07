#pragma once
#include "../config.h"
#include "../pedalState.h"
#include "../statePersistance.h"
#include "../sharedTypes.h"

struct Toggle
{
    uint8_t pin;
    const char *name;
    bool lastState;
};

inline Toggle toggles[] = {
    {MS2_PIN,        "HotSwitch Latching", HIGH},
    {POT_MODE_TOGGLE,"Pot Mode",           HIGH},
    {LST_PIN,        "Lock Settings",      HIGH}};

inline void initToggles(PedalState &pedal)
{
    for (int i = 0; i < 3; i++)
    {
        pinMode(toggles[i].pin, INPUT_PULLUP);
    }
    pedal.modulator.latching = !digitalRead(MS2_PIN);
    pedal.potMode            = digitalRead(POT_MODE_TOGGLE) ? PotMode::RampSpeed : PotMode::SendCC;
    pedal.settingsLocked     = !digitalRead(LST_PIN);
}

inline bool handleToggleChange(Toggle &toggle, PedalState &pedal, void (*displayLockedMessage)(String))
{
    bool state = digitalRead(toggle.pin);

    if (state != toggle.lastState)
    {
        toggle.lastState = state;

        if (pedal.settingsLocked)
        {
            if (toggle.pin == LST_PIN)
            {
                pedal.settingsLocked = !state;
                return true;
            }
            else
            {
                displayLockedMessage("toggles");
                return false;
            }
        }
        else
        {
            if (toggle.pin == MS2_PIN)
            {
                pedal.modulator.setLatch(!state);
            }
            else if (toggle.pin == POT_MODE_TOGGLE)
            {
                pedal.potMode = state ? PotMode::RampSpeed : PotMode::SendCC;
            }
            else if (toggle.pin == LST_PIN)
            {
                pedal.settingsLocked = !state;
            }
            return true;
        }
    }
    return false;
}
