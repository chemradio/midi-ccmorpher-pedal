#pragma once
#include "config.h"
#include "footswitches/footswitchObject.h"
#include "globalSettings.h"
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

    // Saved CC variant (0=Momentary, 1=Latching) during the CC Hi/Lo config levels
    uint8_t modeSelectCCVariant = 0;

    // Three-level mode select (category → sub-group → variant)
    bool    inModeSelect        = false;
    uint8_t modeSelectLevel     = 0;   // 0=category, 1=sub-group/variant, 2=variant within sub-group
    uint8_t modeSelectCatIdx    = 0;
    uint8_t modeSelectVarIdx    = 0;   // sub-group idx at level 1+  (or direct variant if no sub-groups)
    uint8_t modeSelectSubVarIdx = 0;   // variant within sub-group at level 2
    int8_t  modeSelectFSIdx     = -1;

    // Main menu state
    MenuState menuState      = MenuState::NONE;
    uint8_t   menuItemIdx    = 0;
    uint8_t   menuRoutingIdx = 0;   // routing sub-list cursor; also YES/NO in LOCK_CONFIRM

    // Non-preset global settings
    GlobalSettings globalSettings;

    unsigned long rampMinSpeedMs = RAMP_DURATIONS_MIN_MS;
    unsigned long rampMaxSpeedMs = RAMP_DURATIONS_MAX_MS;
    MidiCCModulator modulators[6];

    // Returns the modulator for footswitch idx, or the shared modulator[0]
    // when per-FS mode is disabled (single-modulator / baud-safe mode).
    MidiCCModulator& modForFS(int idx) {
        return globalSettings.perFsModulator ? modulators[idx] : modulators[0];
    }

    std::array<FSButton, 6> buttons = {
        FSButton(FS1_PIN,    PRESET1_LED, "FS 1",    0),
        FSButton(FS2_PIN,    PRESET2_LED, "FS 2",    0),
        FSButton(FS3_PIN,    PRESET3_LED, "FS 3",    0),
        FSButton(FS4_PIN,    PRESET4_LED, "FS 4",    0),
        FSButton(EXTFS1_PIN, PRESET5_LED, "Ext FS 1", 0),
        FSButton(EXTFS2_PIN, PRESET6_LED, "Ext FS 2", 0)};

    void setMidiChannel(uint8_t mc)
    {
        midiChannel = mc;
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

