#ifndef ENCODER_BUTTON_H
#define ENCODER_BUTTON_H
#include "config.h"
#include "footswitchObject.h"
#include "pedalState.h"

inline FSButton encBtn = {ENC_BTN, NO_LED_PIN, "Enc BTN", 0};

inline void initEncoderButton()
{
    pinMode(encBtn.pin, INPUT_PULLUP);
}

inline void handleEncoderButton(PedalState &pedal, void (*displayCallback)(String, String, bool, uint8_t))
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
        encBtn.isActivated = true;
        encBtn.lastState = reading;
        encBtn.lastDebounce = millis();

        int8_t activeButtonIndex = pedal.getActiveButtonIndex();
        Serial.print("Active button index: ");
        Serial.println(activeButtonIndex);

        if (activeButtonIndex >= 0)
        {
            FSButton &activeButton = pedal.buttons[activeButtonIndex];
            Serial.print("Active button: ");
            Serial.println(activeButton.name);
            const char *newMode = activeButton.toggleFootswitchMode();
            Serial.print("New mode: ");
            Serial.println(newMode);
            const char *fsName = activeButton.name;
            Serial.print("FS Name: ");
            Serial.println(fsName);
            bool isPC = activeButton.isPC;
            Serial.print("isPC: ");
            Serial.println(isPC);
            // uint8_t outValue = activeButton.midiNumber;
            // Serial.print("MIDI Number: ");
            // Serial.println(outValue);
            encBtn.lastDebounce = millis();
            displayCallback(fsName, newMode, isPC, activeButton.midiNumber);
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

#endif
