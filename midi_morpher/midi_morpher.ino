
#include "config.h"
#include "pedalState.h"
#include "midiOut.h"
#include "display.h"
#include "toggles.h"
#include "encoder.h"
#include "encoderButton.h"
#include "statePersistance.h"
#include "pots.h"
#include "hotswitch.h"
#include "neopx.h"
#include <USB.h>
#include <USBMIDI.h>

// initialize global state
USBMIDI midi;
PedalState pedal;

void setup()
{
    // Serial.begin(115200); // Initialize serial for debugging
    Serial1.begin(31250, SERIAL_8N1, MIDI_RX, MIDI_TX); // Initialize DIN MIDI serial
    USB.VID(0x303A);                                    // Espressif VID
    USB.PID(0x8000);                                    // Custom PID
    USB.productName("MIDI Morpher 1.0");
    USB.manufacturerName("Tim");
    USB.firmwareVersion(0x0100);
    midi.begin();
    USB.begin();
    delay(1000);
    if (!initDisplay())
    {
        while (1)
            delay(1000);
    }
    initToggles(pedal);
    pedal.initButtons();
    initEncoder();
    initEncoderButton();
    analogReadResolution(12);
    initAnalogPots();
    hotswitch.init();
    loadState(pedal);
    initNeoPixel();
    showStartupScreen();
    delay(2000);
}

void loop()
{
    pedal.ramp.update();
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
    hotswitch.handleHotswitch(pedal.ramp);
    updateNeoPixel(pedal.ramp.currentValue, analogPots);
    resetDisplayTimeout(pedal);
    checkAndSaveState(pedal);
    delay(10);
}
