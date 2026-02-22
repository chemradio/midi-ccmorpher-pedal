#ifndef ENCODER_BUTTON_H
#define ENCODER_BUTTON_H
#include "config.h"
#include "footswitchObject.h"
#include "controlsState.h"
#include "settingsState.h"

using FootswitchDisplayCallback = void (*)(String, String, uint8_t, uint8_t);

inline FSButton encBtn = {ENC_BTN, NO_LED_PIN, "Enc BTN", false, HIGH, 0, false};

inline void initEncoderButton()
{
    pinMode(encBtn.pin, INPUT_PULLUP);
}

inline void handleEncoderButton(ControlsState &controlsState, MIDIMorpherSettingsState &settings, FootswitchDisplayCallback displayCallback)
{
    bool reading = digitalRead(encBtn.pin);
    if ((millis() - encBtn.lastDebounce) < DEBOUNCE_DELAY)
    {
        return;
    }

    if (reading == HIGH)
    {
        controlsState.encoderActivated = false;
    }

    if (reading == LOW && !controlsState.encoderActivated)
    {
        controlsState.encoderActivated = true;
        encBtn.lastState = reading;
        bool isPC = false;
        uint8_t midiValue = 0;
        String btnName = "";

        // mod global state obj
        if (controlsState.fs1Pressed)
        {
            btnName = "FS 1";
            midiValue = settings.fs1Value;
            settings.fs1IsPC = !settings.fs1IsPC;
            isPC = settings.fs1IsPC;
        }
        else if (controlsState.fs2Pressed)
        {
            btnName = "FS 2";
            midiValue = settings.fs2Value;
            settings.fs2IsPC = !settings.fs2IsPC;
            isPC = settings.fs2IsPC;
        }
        else if (controlsState.hotswitchPressed)
        {
            btnName = "HotSwitch";
            midiValue = settings.hotswitchValue;
            settings.hotswitchIsPC = !settings.hotswitchIsPC;
            isPC = settings.hotswitchIsPC;
        }
        else if (controlsState.extfs1Pressed)
        {
            btnName = "ExtFS 1";
            midiValue = settings.extfs1Value;
            settings.extfs1IsPC = !settings.extfs1IsPC;
            isPC = settings.extfs1IsPC;
        }
        else if (controlsState.extfs2Pressed)
        {
            btnName = "ExtFS 2";
            midiValue = settings.extfs2Value;
            settings.extfs2IsPC = !settings.extfs2IsPC;
            isPC = settings.extfs2IsPC;
        }
        else
        {
            // No button held, ignore press
            encBtn.lastDebounce = millis();
            return;
        }

        displayCallback(String(btnName), isPC ? "PC" : "CC", settings.midiChannel, midiValue);
        encBtn.lastDebounce = millis();
        return;
    }
}

#endif
