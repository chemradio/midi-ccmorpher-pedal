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
    int8_t lastActiveFSIndex = -1;    // which footswitch was last pressed (for activity LED)

    unsigned long rampMinSpeedMs = RAMP_DURATIONS_MIN_MS;
    unsigned long rampMaxSpeedMs = RAMP_DURATIONS_MAX_MS;
    MidiCCModulator modulator;

    std::array<FSButton, 6> buttons = {
        FSButton(FS1_PIN,    FS1_LED,    "FS 1",    0),
        FSButton(FS2_PIN,    FS2_LED,    "FS 2",    0),
        FSButton(FS3_PIN,    FS3_LED,    "FS 3",    0),
        FSButton(FS4_PIN,    FS4_LED,    "FS 4",    0),
        FSButton(EXTFS1_PIN, EXTFS1_LED, "Ext FS 1", 0),
        FSButton(EXTFS2_PIN, EXTFS2_LED, "Ext FS 2", 0)};

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

};

