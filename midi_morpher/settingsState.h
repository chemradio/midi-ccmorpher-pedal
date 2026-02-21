#ifndef SETTINGS_STATE_H
#define SETTINGS_STATE_H

struct MIDIMorpherSettingsState
{
    // globals
    uint8_t midiChannel = 1;
    bool rampLinearCurve = true;
    bool rampDirectionUp = true;
    bool hotSwitchLatching = false;
    bool settingsLocked = false;

    // PC or CC + values
    bool fs1IsPC = true;
    bool fs2IsPC = true;
    bool hotswitchIsPC = false;
    bool extfs1IsPC = true;
    bool extfs2IsPC = true;
    // values
    uint8_t fs1Value = 0;
    uint8_t fs2Value = 1;
    uint8_t hotswitchValue = 6;
    uint8_t extfs1Value = 2;
    uint8_t extfs2Value = 3;

    uint8_t rampProgress = 0;
};

inline MIDIMorpherSettingsState currentSettings = {
    .midiChannel = 1,
    .rampLinearCurve = true,
    .rampDirectionUp = true,
    .hotSwitchLatching = false,
    .settingsLocked = false};

#endif