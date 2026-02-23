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

inline void handleEncoderButton(PedalState &pedal, void (*displayCallback)(String, String, uint8_t, uint8_t))
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

        if (activeButtonIndex >= 0)
        {
            const char *newMode = pedal.buttons[activeButtonIndex].toggleFootswitchMode();
            const char *fsName = pedal.buttons[activeButtonIndex].name;
            isPC = pedal.buttons[activeButtonIndex].isPC;
            outValue = pedal.buttons[activeButtonIndex].midiNumber;
            encBtn.lastDebounce = millis();
            displayCallback(String(fsName), newMode, settings.midiChannel, outValue);
            return;
        }
        else
        {
            uint8_t midiChannel = pedal.midiChannel;
            outValue = constrain(midiChannel + delta, 0, 15);
            pedal.midiChannel = outValue;
            isPC = false;
            displayCallback("MIDI Channel", "", pedal.midiChannel, outValue);
            return;
        }
    }
}

#endif
