#pragma once
#include <Preferences.h>

inline Preferences prefs;
inline bool stateDirty = false;
inline unsigned long lastDirtyTime = 0;
inline constexpr unsigned long SAVE_DELAY = 5000;
inline constexpr unsigned long ENCBTN_SAVE_TIMEOUT = 2000;

inline void markStateDirty()
{
    stateDirty = true;
    lastDirtyTime = millis();
}

struct FSButtonPersisted
{
    bool isLatching;
    bool isPC;
    uint8_t midiNumber;
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
        p.buttons[i].isLatching = state.buttons[i].isLatching;
        p.buttons[i].isPC = state.buttons[i].isPC;
        p.buttons[i].midiNumber = state.buttons[i].midiNumber;
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

        state.midiChannel = p.midiChannel;

        for (int i = 0; i < 4; i++)
        {
            state.buttons[i].isLatching = p.buttons[i].isLatching;
            state.buttons[i].isPC = p.buttons[i].isPC;
            state.buttons[i].midiNumber = p.buttons[i].midiNumber;
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
