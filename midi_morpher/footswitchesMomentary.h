#ifndef FOOTSWITCHES_MOMENTARY_H
#define FOOTSWITCHES_MOMENTARY_H
#include "config.h"
#include "footswitchObject.h"
#include "controlsState.h"
#include "settingsState.h"
#include "midiOut.h"

using FootswitchDisplayCallback = void (*)(String, String, uint8_t, uint8_t);

inline FSButton momentaryFSButtons[] = {
    {FS1_PIN, FS1_LED, "FS 1"},
    {FS2_PIN, FS2_LED, "FS 2"},
    {EXTFS1_PIN, EXTFS1_LED, "extFS 1"},
    {EXTFS2_PIN, EXTFS2_LED, "extFS 2"},
};

inline void initMomentaryFootswitches()
{
    for (int i = 0; i < 4; i++)
    {
        momentaryFSButtons[i].init();
    }
}

inline void handleMomentaryFootswitch(FSButton &btn, ControlsState &controlsState, MIDIMorpherSettingsState &settings, FootswitchDisplayCallback displayCallback)
{
    bool reading = digitalRead(btn.pin);
    if ((millis() - btn.lastDebounce) < DEBOUNCE_DELAY)
    {
        return;
    }

    if (reading != btn.lastState)
    {
        btn.lastState = reading;
        btn.setLED(!reading);

        bool isPC = false;
        uint8_t midiValue = 0;

        // mod global state obj
        if (btn.pin == FS1_PIN)
        {
            controlsState.fs1Pressed = !reading;
            midiValue = settings.fs1Value;
            isPC = settings.fs1IsPC;
        }
        else if (btn.pin == FS2_PIN)
        {
            controlsState.fs2Pressed = !reading;
            midiValue = settings.fs2Value;
            isPC = settings.fs2IsPC;
        }
        else if (btn.pin == HS_PIN)
        {
            controlsState.hotswitchPressed = !reading;
            midiValue = settings.hotswitchValue;
            isPC = settings.hotswitchIsPC;
        }
        else if (btn.pin == EXTFS1_PIN)
        {
            controlsState.extfs1Pressed = !reading;
            midiValue = settings.extfs1Value;
            isPC = settings.extfs1IsPC;
        }
        else if (btn.pin == EXTFS2_PIN)
        {
            controlsState.extfs2Pressed = !reading;
            midiValue = settings.extfs2Value;
            isPC = settings.extfs2IsPC;
        }

        displayCallback(String(btn.name), isPC ? "PC" : "CC", settings.midiChannel, midiValue);

        if (!reading)
        {
            sendMIDI(settings.midiChannel, isPC, midiValue, !reading ? 127 : 0);
        }

        btn.lastDebounce = millis();
        return;
    }
}

#endif
