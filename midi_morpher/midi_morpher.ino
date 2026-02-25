// #include "footswitchesMomentary.h"
#include "config.h"
#include "pedalState.h"
#include "midiOut.h"
#include "display.h"
#include "toggles.h"
#include "encoder.h"
#include "encoderButton.h"
#include "statePersistance.h"
#include "pots.h"

// initialize global state
PedalState pedal;
// Toggle &hsLatchToggle = toggles[0];
// Toggle &rampDirToggle = toggles[1];
// Toggle &rampCurveToggle = toggles[2];
// Toggle &lockSettingsToggle = toggles[3];

void setup()
{
    Serial.begin(115200);                               // Initialize serial for debugging
    Serial1.begin(31250, SERIAL_8N1, MIDI_RX, MIDI_TX); // Initialize DIN MIDI serial
    // // usbMIDI.begin();
    if (!initDisplay())
    {
        while (1)
            delay(1000);
    }
    showStartupScreen();
    initToggles(pedal);
    pedal.initButtons();
    initEncoder();
    initEncoderButton();
    analogReadResolution(12);
    initAnalogPots();
    loadState(pedal);
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
        bool toggleChanged = handleToggleChange(tgl, pedal, displayLockedMessage);
        if (toggleChanged)
        {
            displayHomeScreen(pedal);
        }
    }
    for (auto &pot : analogPots)
    {
        handleAnalogPot(pot, pedal, displayPotValue, displayLockedMessage);
    }
    handleEncoder(pedal, displayEncoderTurn, displayLockedMessage);
    handleEncoderButton(pedal, encoderButtonFSModeChange, displayLockedMessage);
    resetDisplayTimeout(pedal);
    checkAndSaveState(pedal);
    // tempDisplayPotValue(potValue);
    delay(10);
}
