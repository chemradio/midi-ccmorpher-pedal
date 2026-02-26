#ifndef POTS_H
#define POTS_H
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
        analogPots[i].lastValue = medianRead;
        analogPots[i].lastMidiValue = (medianRead * 128UL) / 4096;
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

        long rampMs = map(selectedValue, 0, 4095, pedal.rampMinSpeedMs, pedal.rampMaxSpeedMs);

        bool isMidiCC = false;
        String potDisplayName = "";

        if (pedal.potMode == PotMode::SendCC)
        {
            if (midiScaled != pot.lastMidiValue)
            {
                pot.lastMidiValue = midiScaled;
                isMidiCC = true;
                potDisplayName = "MidiCC: " + String(pot.midiCCNumber);
                pot.sendMidiCC(pedal.midiChannel);
            }
        }
        else
        {
            if (pot.pin == POT1_PIN)
            {
                pedal.ramp.rampUpMs = rampMs;
            }
            else if (pot.pin == POT2_PIN)
            {
                pedal.ramp.rampDownMs = rampMs;
            }
            potDisplayName = pot.name;
        }

        displayCallback(
            potDisplayName,
            isMidiCC,
            midiScaled,
            rampMs);
    }
}
// inline void handleAnalogPot(AnalogPot &pot, SignalCallback cb)
// {
//     const float alpha = 0.18f; // smaller = smoother, larger = faster

//     uint16_t raw = analogRead(pot.pin);

//     // exponential smoothing
//     pot.filtered += alpha * (raw - pot.filtered);

//     uint16_t smooth = (uint16_t)pot.filtered;

//     // deadband against filtered value
//     if (abs((int)smooth - (int)pot.lastValue) > potDeadband)
//     {
//         pot.lastValue = smooth;

//         cb(
//             String(pot.name),
//             smooth,
//             String(map(smooth, 0, 4095, 0, 127)));
//     }
// }

// inline void handleAnalogPot(AnalogPot &pot, SignalCallback cb)
// {
//     uint16_t value = analogRead(pot.pin);

//     if (abs(value - pot.lastValue) > potDeadband)
//     {
//         pot.lastValue = value;
//         cb(String(pot.name), value, String(map(value, 0, 4095, 0, 127)));
//     }
// }

#endif