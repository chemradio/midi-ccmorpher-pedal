#pragma once
#include <Preferences.h>

inline Preferences prefs;
inline bool stateDirty = false;
inline unsigned long lastDirtyTime = 0;
inline constexpr unsigned long SAVE_DELAY = 5000;

inline void markStateDirty()
{
    stateDirty = true;
    lastDirtyTime = millis();
}

struct FSButtonPersisted
{
    uint8_t  modeIndex;
    uint8_t  midiNumber;
    uint32_t rampUpMs;
    uint32_t rampDownMs;
    uint8_t  fsChannel;  // 0xFF = follow global; 0–15 = per-FS override
};

struct PedalPersisted
{
    uint8_t midiChannel;
    FSButtonPersisted buttons[4];
};

inline void saveState(const PedalState &state)
{
    PedalPersisted p;
    p.midiChannel = state.midiChannel;
    for (int i = 0; i < 4; i++)
    {
        p.buttons[i].modeIndex  = state.buttons[i].modeIndex;
        p.buttons[i].midiNumber = state.buttons[i].midiNumber;
        p.buttons[i].rampUpMs   = state.buttons[i].rampUpMs;
        p.buttons[i].rampDownMs = state.buttons[i].rampDownMs;
        p.buttons[i].fsChannel  = state.buttons[i].fsChannel;
    }
    prefs.begin("pedal", false);
    prefs.putBytes("cfg", &p, sizeof(p));
    prefs.end();
}

inline void loadState(PedalState &state)
{
    PedalPersisted p;
    prefs.begin("pedal", true);

    if (prefs.getBytesLength("cfg") == sizeof(p))
    {
        prefs.getBytes("cfg", &p, sizeof(p));

        state.setMidiChannel(constrain(p.midiChannel, 0, 15));

        for (int i = 0; i < 4; i++)
        {
            uint8_t idx = p.buttons[i].modeIndex < NUM_MODES ? p.buttons[i].modeIndex : 0;
            const ModeInfo &m = modes[idx];
            FSButton &btn = state.buttons[i];

            btn.modeIndex  = idx;
            btn.midiNumber = p.buttons[i].midiNumber;
            btn.rampUpMs   = p.buttons[i].rampUpMs;
            btn.rampDownMs = p.buttons[i].rampDownMs;
            btn.fsChannel  = p.buttons[i].fsChannel;

            // Apply all mode-derived flags from the modes table
            btn.mode        = m.mode;
            btn.isLatching  = m.isLatching;
            btn.isPC        = m.isPC;
            btn.isNote      = m.isNote;
            btn.isScene     = m.isScene;
            btn.isModSwitch = m.isModSwitch;
            btn.modMode     = m;
        }
    }

    prefs.end();
}

inline void checkAndSaveState(const PedalState &state)
{
    if (stateDirty && (millis() - lastDirtyTime) >= SAVE_DELAY)
    {
        saveState(state);
        stateDirty = false;
    }
}
