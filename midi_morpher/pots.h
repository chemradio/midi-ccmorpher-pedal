#pragma once
#include "config.h"
#include "pedalState.h"
#include "midiOut.h"

inline const uint8_t potDeadband = 20;
unsigned long lastSettingsBlockedDisplayed = 0;
unsigned long settingsBlockedDisplayTimeout = 1000;

struct AnalogPot
{
    uint8_t pin;
    const char *name;
    uint8_t midiCCNumber;
    uint16_t lastValue = 0;
    uint8_t lastMidiValue = 0;
    bool ccDisplayDirty = false;
    long ccLastDisplayDirty = 0;

    void sendMidiCC(uint8_t midiChannel)
    {
        sendMIDI(midiChannel, false, midiCCNumber, lastMidiValue);
    }
};

inline AnalogPot analogPots[] = {
    {POT1_PIN, "UP Speed", POT1_CC},
    {POT2_PIN, "Down Speed", POT2_CC}};

uint16_t readPotMedian(uint8_t pin)
{
    uint16_t a = analogRead(pin);
    uint16_t b = analogRead(pin);
    uint16_t c = analogRead(pin);
    uint16_t minv = min(a, min(b, c));
    uint16_t maxv = max(a, max(b, c));
    return a + b + c - minv - maxv; // middle value
}

uint16_t readPotAvg(uint8_t pin)
{
    uint32_t sum = 0;
    for (int i = 0; i < 4; i++)
    {
        sum += analogRead(pin);
    }
    return sum >> 2; // divide by 4
}

inline void initAnalogPots()
{
    for (int i = 0; i < 2; i++)
    {
        pinMode(analogPots[i].pin, INPUT);
        uint16_t medianRead = readPotMedian(analogPots[i].pin);
        // analogPots[i].lastValue = medianRead;
        // analogPots[i].lastMidiValue = (medianRead * 128UL) / 4096;
    }
}

inline void handleAnalogPot(AnalogPot &pot, PedalState &pedal, void (*displayCallback)(String, bool, uint8_t, long), void (*displayLockedMessage)(String))
{

    // uint16_t raw = analogRead(pot.pin);
    uint16_t selectedValue = readPotMedian(pot.pin);

    if (abs((int)selectedValue - (int)pot.lastValue) > potDeadband)
    {

        pot.lastValue = selectedValue;
        uint8_t midiScaled = (selectedValue * 128UL) / 4096;

        if (pedal.settingsLocked)
        {
            unsigned long now = millis();
            if ((now - lastSettingsBlockedDisplayed) > settingsBlockedDisplayTimeout)
            {
                displayLockedMessage("pots");
                lastSettingsBlockedDisplayed = now;
            }
            return;
        }

        // clamp to ADC range
        if (selectedValue > 4095)
            selectedValue = 4095;

        if (pedal.potMode == PotMode::SendCC)
        {
            if (midiScaled != pot.lastMidiValue)
            {
                pot.lastMidiValue = midiScaled;
                pot.sendMidiCC(pedal.midiChannel);
                pot.ccDisplayDirty = true;
                pot.ccLastDisplayDirty = millis();

                displayCallback(
                    "MidiCC: " + String(pot.midiCCNumber),
                    true,
                    midiScaled,
                    0);
            }
        }
        else
        {
            // linear pot feel
            // long rampMs = map(selectedValue, 0, 4095, pedal.rampMinSpeedMs, pedal.rampMaxSpeedMs);

            // exponential pot feel
            float normalized = selectedValue / 4095.0;
            float curved = normalized * normalized;
            long rampMs = pedal.rampMinSpeedMs + (curved * (pedal.rampMaxSpeedMs - pedal.rampMinSpeedMs));

            if (pot.pin == POT1_PIN)
            {
                pedal.ramp.setRampTimeUp(rampMs);
            }
            else if (pot.pin == POT2_PIN)
            {
                pedal.ramp.setRampTimeDown(rampMs);
            }
            displayCallback(
                pot.name,
                false,
                midiScaled,
                rampMs);
        }
    }
}