#pragma once
#include <stdlib.h> // rand()

// RandomStepper additional state — add these to MidiCCModulator if not present:
//   unsigned long randomIntervalMs = 300; // ms between random jumps
//   unsigned long lastRandomTime   = 0;
//   uint8_t randomMin = 0;               // lower bound for random target (0-127)
//   uint8_t randomMax = 127;             // upper bound for random target (0-127)

// Interval model:
//   When jumping UP   the wait before the next jump uses rampUpTimeMs.
//   When jumping DOWN the wait before the next jump uses rampDownTimeMs.
//   On release (momentary) or latch-off: snaps immediately to 0.

inline void MidiCCModulator::updateRandomStepper()
{
    // --- Snap to 0 on release ---
    if (!isActivated)
    {
        if (currentValue != 0)
        {
            currentValue = 0;
            sendMIDI(midiChannel, false, midiCCNumber, currentValue);
        }
        isModulating = false;
        return;
    }

    unsigned long now = millis();

    // Choose interval based on last direction of travel
    unsigned long interval = (currentValue <= randomMin)
                                 ? rampUpTimeMs
                             : (currentValue >= randomMax)
                                 ? rampDownTimeMs
                                 : randomIntervalMs; // fallback (centre range)

    if (now - lastRandomTime < interval)
        return;

    lastRandomTime = now;

    // Pick a new random value within [randomMin, randomMax]
    uint8_t range = (randomMax > randomMin) ? (randomMax - randomMin) : 0;

    if (range == 0)
    {
        // Min and max are the same — nothing to randomise
        currentValue = randomMin;
        sendMIDI(midiChannel, false, midiCCNumber, currentValue);
        return;
    }

    uint8_t newVal = randomMin + (uint8_t)(rand() % (range + 1));
    uint8_t attempts = 0;

    // Avoid immediate repetition (up to 8 tries)
    while (newVal == currentValue && attempts < 8)
    {
        newVal = randomMin + (uint8_t)(rand() % (range + 1));
        attempts++;
    }

    // Use rampUpTimeMs / rampDownTimeMs as the interval AFTER this jump,
    // based on the direction we just moved
    // (lastRandomTime was already updated above, so next interval is set next call)

    currentValue = newVal;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
}
