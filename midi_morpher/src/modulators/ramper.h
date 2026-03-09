#pragma once

inline void MidiCCModulator::updateRamper()
{

    if (currentRampDuration <= 1)
    {
        currentValue = targetValue;
        isModulating = false;
        sendMIDI(midiChannel, false, midiCCNumber, currentValue);
        return;
    }

    unsigned long now = millis();
    unsigned long elapsed = now - rampStartTime;

    if (elapsed >= currentRampDuration)
    {
        currentValue = targetValue;
        isModulating = false;
        sendMIDI(midiChannel, false, midiCCNumber, currentValue);
        return;
    }

    float t = (float)elapsed / currentRampDuration;
    if (t > 1.0f)
        t = 1.0f;

    float shaped;

    if (!isLinear)
    {
        shaped = t * t;
        // float inv = 1.0f - t;
        // shaped = 1.0f - (inv * inv * inv);
    }
    else // linear
    {
        shaped = t;
    }

    int delta = targetValue - rampStartValue;
    uint8_t newValue = rampStartValue + (delta * shaped);

    if (newValue != currentValue)
    {
        currentValue = newValue;
        sendMIDI(midiChannel, false, midiCCNumber, currentValue);
    }
}