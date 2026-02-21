#include "config.h"
#include "display.h"
#include "footswitchesMomentary.h"
#include "toggles.h"
#include "encoder.h"
#include "pots.h"
#include "settingsState.h"
#include "controlsState.h"
#include "encoderButton.h"

MIDIMorpherSettingsState currentSettings = {
    .midiChannel = 1,
    .rampLinearCurve = true,
    .rampDirectionUp = true,
    .hotSwitchLatching = false,
    .settingsLocked = false};

ControlsState controlsState;

void showHomeScreen()
{
    displayHomeScreen(currentSettings);
}

void setup()
{
    Serial.begin(9600);
    Serial.print("EXTFS2_PIN state: ");
    Serial.println(digitalRead(EXTFS2_PIN));
    delay(1000);

    if (!initDisplay())
    {
        Serial.println("FAIL: Display init failed!");
        while (1)
            delay(1000);
    }

    Serial1.begin(31250, SERIAL_8N1, MIDI_RX, MIDI_TX); // Initialize DIN MIDI serial
    // usbMIDI.begin();
    initMomentaryFootswitches();
    initToggles();
    initEncoderButton();
    initEncoder();
    initAnalogPots();
    analogReadResolution(12);
    showStartupScreen();
    delay(2000);
}

void loop()
{
    for (auto &btn : momentaryFSButtons)
    {
        handleMomentaryFootswitch(btn, controlsState, currentSettings, displayFootswitchPress);
    }
    for (auto &tgl : toggles)
    {
        bool toggleChanged = handleToggle(tgl, currentSettings, controlsState, displayToggleChange);
        if (toggleChanged)
        {
            showHomeScreen();
        }
    }
    handleEncoderButton(controlsState, currentSettings, displayFootswitchPress);
    handleEncoder(currentSettings, controlsState, displayEncoderTurn);

    // for (auto &pot : analogPots)
    // {
    //     handleAnalogPot(pot, controlsState, showSignal);
    // }
    resetDisplayTimeout(currentSettings);
    delay(10);
}
