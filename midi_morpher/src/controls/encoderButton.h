#pragma once
#include "../config.h"
#include "../footswitches/footswitchObject.h"
#include "../pedalState.h"
#include "../statePersistance.h"

inline FSButton encBtn = {ENC_BTN, NO_LED_PIN, "Enc BTN", 0};

inline void initEncoderButton() {
  pinMode(encBtn.pin, INPUT_PULLUP);
}

inline void handleEncoderButton(PedalState &pedal, void (*displayCallback)(FSButton &), MidiCCModulator &modulator, void (*displayLockedMessage)(String)) {

  // debounce
  if((millis() - encBtn.lastDebounce) < DEBOUNCE_DELAY)
    return;

  bool reading = digitalRead(encBtn.pin);
  if(reading == HIGH) {
    encBtn.isActivated = false;
    return;
  }

  if(reading == LOW && !encBtn.isActivated) {
    if(pedal.settingsLocked) {
      displayLockedMessage("encBtn");
      return;
    }

    encBtn.isActivated = true;
    encBtn.lastState = reading;
    encBtn.lastDebounce = millis();

    int8_t activeButtonIndex = pedal.getActiveButtonIndex();

    if(activeButtonIndex >= 0) {
      FSButton &activeButton = pedal.buttons[activeButtonIndex];
      activeButton.toggleFootswitchMode(modulator);
      encBtn.lastDebounce = millis();
      displayCallback(activeButton);
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
