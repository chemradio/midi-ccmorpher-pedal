#ifndef HOTSWITCH_H
#define HOTSWITCH_H
#include "config.h"
#include "footswitchObject.h"
#include "controlsState.h"
#include "settingsState.h"

using FootswitchDisplayCallback = void (*)(String, String, uint8_t, uint8_t);

inline FSButton hotswitch = {FS2_PIN, FS2_LED, "FS 2", false, HIGH, 0, false};

inline void initHotswitch()
{
    pinMode(hotswitch.pin, INPUT_PULLUP);
    pinMode(hotswitch.ledPin, OUTPUT);
    digitalWrite(hotswitch.ledPin, LOW);
}

inline void handleHotswitch(MIDIMorpherSettingsState &settings, ControlsState &controlsState, FootswitchDisplayCallback displayCallback)
{
    if (settings.hotSwitchLatching)
        handleHotswitchLatching();
    else
        handleHotswitchMomentary();
}

void handleHotswitchLatching(...);

void handleHotswitchMomentary(...);
{
    bool reading = digitalRead(hotswitch.pin);
    if ((millis() - hotswitch.lastDebounce) < DEBOUNCE_DELAY)
    {
        return;
    }

    // if (reading != btn.lastState)
    // {
    //     btn.lastState = reading;
    //     digitalWrite(btn.ledPin, !reading ? HIGH : LOW);

    //     bool isPC = false;
    //     uint8_t midiValue = 0;

    //     // mod global state obj
    //     if (btn.pin == FS1_PIN)
    //     {
    //         controlsState.fs1Pressed = !reading;
    //         midiValue = settings.fs1Value;
    //         isPC = settings.fs1IsPC;
    //     }
    //     else if (btn.pin == FS2_PIN)
    //     {
    //         controlsState.fs2Pressed = !reading;
    //         midiValue = settings.fs2Value;
    //         isPC = settings.fs2IsPC;
    //     }
    //     else if (btn.pin == HS_PIN)
    //     {
    //         controlsState.hotswitchPressed = !reading;
    //         midiValue = settings.hotswitchValue;
    //         isPC = settings.hotswitchIsPC;
    //     }
    //     else if (btn.pin == EXTFS1_PIN)
    //     {
    //         controlsState.extfs1Pressed = !reading;
    //         midiValue = settings.extfs1Value;
    //         isPC = settings.extfs1IsPC;
    //     }
    //     else if (btn.pin == EXTFS2_PIN)
    //     {
    //         controlsState.extfs2Pressed = !reading;
    //         midiValue = settings.extfs2Value;
    //         isPC = settings.extfs2IsPC;
    //     }

    //     displayCallback(String(btn.name), isPC ? "PC" : "CC", settings.midiChannel, midiValue);
    //     btn.lastDebounce = millis();
    //     return;
    // }
}

#endif
