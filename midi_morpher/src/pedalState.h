#pragma once
#include "config.h"
#include "footswitches/footswitchObject.h"
#include "midiCCModulator.h"
#include "sharedTypes.h"

struct PedalState
{
    // pedal globals
    uint8_t midiChannel = 0;
    bool settingsLocked = false;
    int8_t activeButtonIndex = -1;

    bool   inChannelSelect  = false;  // encoder is currently editing a per-FS MIDI channel
    int8_t channelSelectIdx = -1;     // which button is being edited

    unsigned long rampMinSpeedMs = RAMP_DURATIONS_MIN_MS;
    unsigned long rampMaxSpeedMs = RAMP_DURATIONS_MAX_MS;
    MidiCCModulator modulator;

    std::array<FSButton, 4> buttons = {
        FSButton(FS1_PIN, FS1_LED, "FS 1", 0),
        FSButton(FS2_PIN, FS2_LED, "FS 2", 1),
        FSButton(EXTFS1_PIN, EXTFS1_LED, "extFS 1", 2),
        FSButton(EXTFS2_PIN, EXTFS2_LED, "extFS 2", 3)};

    void setMidiChannel(uint8_t mc)
    {
        midiChannel = mc;
        modulator.midiChannel = mc;
    }

    void init()
    {
        initButtons();
    }

    void initButtons()
    {
        for (auto &button : buttons)
        {
            button.init();
        }
    }

    int8_t getActiveButtonIndex()
    {
        for (size_t i = 0; i < buttons.size(); i++)
        {
            if (buttons[i].isPressed)
            {
                return i;
            }
        }
        return -1; // no button pressed
    }

    void adjustActiveMidiNumber(int8_t delta)
    {
        if (activeButtonIndex >= 0 && activeButtonIndex < buttons.size())
        {
            FSButton &btn = buttons[activeButtonIndex];
            int newValue = btn.midiNumber + delta;

            // Clamp to valid MIDI range (0-127)
            if (newValue < 0)
                newValue = 0;
            if (newValue > 127)
                newValue = 127;

            btn.midiNumber = newValue;
        }
    }


};
