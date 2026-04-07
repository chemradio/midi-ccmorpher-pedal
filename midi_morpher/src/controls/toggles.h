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
    {MS2_PIN, "HotSwitch Latching", HIGH},
    {LST_PIN, "Lock Settings",      HIGH}};

inline void initToggles(PedalState &pedal)
{
    for (auto &tgl : toggles)
    {
        pinMode(tgl.pin, INPUT_PULLUP);
        tgl.lastState = digitalRead(tgl.pin); // sync to actual state — prevents spurious first-loop triggers
    }
    pedal.modulator.latching = !digitalRead(MS2_PIN);
    pedal.settingsLocked     = !digitalRead(LST_PIN);
}

inline bool handleToggleChange(Toggle &toggle, PedalState &pedal,
                               void (*displayLockedMessage)(String),
                               void (*displayLockChange)(bool))
{
    bool state = digitalRead(toggle.pin);
    if (state == toggle.lastState) return false;
    toggle.lastState = state;

    // Lock switch always works — even when locked, you can unlock.
    if (toggle.pin == LST_PIN)
    {
        pedal.settingsLocked = !state; // LOW = locked
        displayLockChange(pedal.settingsLocked);
        return false; // lock display already handled — caller need not refresh home screen
    }

    // All other toggles are blocked when locked.
    if (pedal.settingsLocked)
    {
        displayLockedMessage("toggles");
        return false;
    }

    if (toggle.pin == MS2_PIN)
    {
        pedal.modulator.setLatch(!state);
    }

    return true; // caller should refresh home screen
}
