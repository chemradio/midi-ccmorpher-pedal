// #include "footswitchesMomentary.h"
// #include "pots.h"
#include "config.h"
#include "pedalState.h"
#include "midiOut.h"
#include "display.h"
#include "toggles.h"
#include "encoder.h"
#include "encoderButton.h"

// initialize global state
PedalState pedal;

void setup()
{
    Serial.begin(115200); // Initialize serial for debugging
    // Serial1.begin(31250, SERIAL_8N1, MIDI_RX, MIDI_TX); // Initialize DIN MIDI serial
    // usbMIDI.begin();
    if (!initDisplay())
    {
        // Serial.println("FAIL: Display init failed!");
        while (1)
            delay(1000);
    }
    showStartupScreen();

    pedal.initButtons();
    initToggles();

    initEncoder();
    initEncoderButton();

    // initAnalogPots();
    // analogReadResolution(12);
    delay(2000);
}

void loop()
{
    for (auto &button : pedal.buttons)
    {
        button.handleFootswitch(pedal.midiChannel, displayFootswitchPress);
    }

    for (auto &tgl : toggles)
    {
        bool toggleChanged = handleToggleChange(tgl, pedal);
        if (toggleChanged)
        {
            displayHomeScreen(pedal);
        }
    }
    handleEncoder(pedal, displayEncoderTurn);
    handleEncoderButton(pedal, encoderButtonFSModeChange);

    // // for (auto &pot : analogPots)
    // // {
    // //     handleAnalogPot(pot, controlsState, showSignal);
    // // }
    resetDisplayTimeout(pedal);
    delay(10);
}
