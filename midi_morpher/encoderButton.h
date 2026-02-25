#pragma once
#include "config.h"
#include "footswitchObject.h"
#include "pedalState.h"
#include "statePersistance.h"

inline FSButton encBtn = {ENC_BTN, NO_LED_PIN, "Enc BTN", 0};

inline void initEncoderButton()
{
    pinMode(encBtn.pin, INPUT_PULLUP);
}

inline void handleEncoderButton(PedalState &pedal, void (*displayCallback)(String, String, bool, uint8_t), void (*displayLockedMessage)(String))
{

    // debounce
    if ((millis() - encBtn.lastDebounce) < DEBOUNCE_DELAY)
        return;

    bool reading = digitalRead(encBtn.pin);
    if (reading == HIGH)
    {
        encBtn.isActivated = false;
        return;
    }

    if (reading == LOW && !encBtn.isActivated)
    {
        if (pedal.settingsLocked)
        {
            displayLockedMessage("encBtn");
            return;
        }

        encBtn.isActivated = true;
        encBtn.lastState = reading;
        encBtn.lastDebounce = millis();

        int8_t activeButtonIndex = pedal.getActiveButtonIndex();
        Serial.print("Active button index: ");
        Serial.println(activeButtonIndex);

        if (activeButtonIndex >= 0)
        {
            FSButton &activeButton = pedal.buttons[activeButtonIndex];
            const char *newMode = activeButton.toggleFootswitchMode();
            const char *fsName = activeButton.name;
            bool isPC = activeButton.isPC;
            encBtn.lastDebounce = millis();
            displayCallback(fsName, newMode, isPC, activeButton.midiNumber);
            markStateDirty();
            return;
        }
        // else
        // {
        //     uint8_t midiChannel = pedal.midiChannel;
        //     uint8_t outValue = constrain(midiChannel + delta, 0, 15);
        //     pedal.midiChannel = outValue;
        //     bool isPC = false;
        //     displayCallback("MIDI Channel", "", pedal.midiChannel, outValue);
        //     return;
        // }
    }
}
