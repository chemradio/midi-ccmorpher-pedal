#pragma once
#include "config.h"
#include "footswitchObject.h"

enum class PotMode
{
    RampSpeed,
    SendCC
};

struct PedalState
{
    // pedal globals
    uint8_t midiChannel = 0;
    bool rampLinearCurve = true;
    bool rampDirectionUp = true;
    bool hotSwitchLatching = false;
    PotMode potMode = PotMode::RampSpeed;
    bool settingsLocked = false;

    unsigned long rampUpSpeed = 1.0;
    unsigned long rampDownSpeed = 1.0;
    unsigned long rampMinSpeedSeconds = 0;
    unsigned long rampMaxSpeedSeconds = 5000;

    int8_t activeButtonIndex = -1;

    // temporary states for ramp
    uint8_t rampProgress = 0; // 0-255
    uint8_t rampProgressMIDI = 0;
    bool ramping = false;
    bool rampingUp = false;

    std::array<FSButton, 4> buttons = {
        FSButton(FS1_PIN, FS1_LED, "FS 1", 0),
        FSButton(FS2_PIN, FS2_LED, "FS 2", 1),
        FSButton(EXTFS1_PIN, EXTFS1_LED, "extFS 1", 2),
        FSButton(EXTFS2_PIN, EXTFS2_LED, "extFS 2", 3)};

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

    const char *getPotMode()
    {
        switch (potMode)
        {
        case PotMode::RampSpeed:
            return "Ramp Speed";
        case PotMode::SendCC:
            return "Send CC";
        }
    }

    const char *togglePotMode()
    {
        switch (potMode)
        {
        case PotMode::RampSpeed:
            potMode = PotMode::SendCC;
            return "Send CC";
        case PotMode::SendCC:
            potMode = PotMode::RampSpeed;
            return "Ramp Speed";
        }
        return "unknown";
    }
};
