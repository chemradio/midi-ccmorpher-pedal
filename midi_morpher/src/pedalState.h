#pragma once
#include "config.h"
#include "footswitches/footswitch.h"
#include "globalSettings.h"
#include "modulators/ccModulator.h"
#include "sharedTypes.h"

static constexpr uint8_t FS_LOAD_ACTION_IDX = 6;

// Pedal-wide preset-nav event flags. Set by FS handlers; consumed and cleared
// by the main loop. Lives here (not in footswitchObject.h) because it is global
// pedal state, not per-button state.
inline int8_t presetNavRequest = 0;  // +1 = PresetUp, -1 = PresetDown, 0 = none
inline int8_t presetNavDirect  = -1; // 0..NUM_PRESETS-1 = jump, -1 = none

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

    // Three-level mode select (category → sub-group → variant → value → velocity)
    bool    inModeSelect        = false;
    uint8_t modeSelectLevel     = 0;   // 0=cat, 1=sub/var, 2=var/CChi, 3=CClo, 4=value, 5=velocity
    uint8_t modeSelectCatIdx    = 0;
    uint8_t modeSelectVarIdx    = 0;   // sub-group idx at level 1+ / value at level 4
    uint8_t modeSelectSubVarIdx = 0;   // variant within sub-group at level 2
    int8_t  modeSelectFSIdx     = -1;  // -1=none; 0-5=FS index; 6=load action edit
    uint8_t modeSelectFinalIdx  = 0;   // tentative mode index when entering level 4+
    uint8_t modeSelectVelocity  = 100; // velocity scratch at level 5
    uint32_t modeSelectRampUp   = DEFAULT_RAMP_SPEED; // ramp speed scratch at levels 6/7
    uint32_t modeSelectRampDown = DEFAULT_RAMP_SPEED;

    // Action selector (FS+encBtn → pick which press-type to configure)
    bool    inActionSelect            = false;
    uint8_t actionSelectSlot          = 0;   // highlighted slot
    bool    modeSelectFromActionSelect = false; // return-to-selector after mode select
    int8_t  modeSelectExtraActionType = -1;  // -1=PRESS, 0/1/2=extraActions[t]

    // FS cursor edit menu
    bool    inFSEdit        = false;
    uint8_t fsEditFSIdx     = 0;      // 0-5=FS; FS_LOAD_ACTION_IDX=load action
    int8_t  fsEditExtraType = -1;     // -1=PRESS; 0=HOLD; 1=DBL; 2=RELEASE
    uint8_t fsEditCursor    = 0;
    bool    fsEditEditing   = false;

    FSButton& fsEditTarget() {
        return (fsEditExtraType >= 0 || fsEditFSIdx == FS_LOAD_ACTION_IDX)
            ? loadActionEditBtn : buttons[fsEditFSIdx];
    }
    MidiCCModulator& fsEditMod() {
        if(fsEditFSIdx == FS_LOAD_ACTION_IDX) return modulators[0];
        return modForFS((int)fsEditFSIdx);
    }
    void fsEditApplyMode(uint8_t newModeIdx) {
        if(fsEditFSIdx == FS_LOAD_ACTION_IDX || fsEditExtraType >= 0)
            applyModeIndex(loadActionEditBtn, newModeIdx, nullptr);
        else
            applyModeIndex(buttons[fsEditFSIdx], newModeIdx, &fsEditMod());
    }

    // Main menu state
    MenuState menuState      = MenuState::NONE;
    uint8_t   menuItemIdx    = 0;
    uint8_t   menuRoutingIdx = 0;   // routing sub-list cursor; also YES/NO in LOCK_CONFIRM

    // Non-preset global settings
    GlobalSettings globalSettings;

    unsigned long rampMinSpeedMs = RAMP_DURATIONS_MIN_MS;
    unsigned long rampMaxSpeedMs = RAMP_DURATIONS_MAX_MS;
    MidiCCModulator modulators[6];

    MidiCCModulator& modForFS(int idx) {
        return globalSettings.perFsModulator ? modulators[idx] : modulators[0];
    }
    const MidiCCModulator& modForFS(int idx) const {
        return globalSettings.perFsModulator ? modulators[idx] : modulators[0];
    }

    // Virtual button used when editing the preset load action via mode select.
    FSButton loadActionEditBtn = FSButton(255, "Load Act", 0);

    // Live (editable) copy of the active preset's load action. Mirrors how
    // pedal.buttons[] works: edits land here and only flush to presets[]/disk
    // on saveCurrentPreset(). applyPreset() reseeds this from the loaded slot.
    FSActionPersisted liveLoadAction = {};

    // Returns the FSButton being edited in mode select (loadActionEditBtn when FSIdx==6).
    FSButton& modeSelectBtn() {
        return (modeSelectFSIdx == 6)
            ? loadActionEditBtn
            : buttons[(uint8_t)modeSelectFSIdx];
    }

    // Returns the modulator to use for mode select (modulators[0] for load action).
    MidiCCModulator& modeSelectMod() {
        return (modeSelectFSIdx == 6)
            ? modulators[0]
            : (globalSettings.perFsModulator ? modulators[(uint8_t)modeSelectFSIdx] : modulators[0]);
    }

    std::array<FSButton, 6> buttons = {
        FSButton(FS1_PIN,    "FS 1",     0),
        FSButton(FS2_PIN,    "FS 2",     0),
        FSButton(FS3_PIN,    "FS 3",     0),
        FSButton(FS4_PIN,    "FS 4",     0),
        FSButton(EXTFS1_PIN, "Ext FS 1", 0),
        FSButton(EXTFS2_PIN, "Ext FS 2", 0)};

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

    // Centralized exit from any UI mode/menu/edit screen back to home.
    // Call sites (FS press, display timeout, encoder confirm) used to clear
    // each flag individually — easy to forget one. Keep them in sync here.
    bool anyUIModeActive() const
    {
        return inModeSelect || inActionSelect || inChannelSelect || inFSEdit ||
               menuState != MenuState::NONE;
    }

    void exitAllUIModes()
    {
        inModeSelect              = false;
        inActionSelect            = false;
        inChannelSelect           = false;
        inFSEdit                  = false;
        fsEditEditing             = false;
        modeSelectFromActionSelect = false;
        menuState                 = MenuState::NONE;
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

