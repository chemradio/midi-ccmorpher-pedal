#pragma once

// Stepper additional state — add these to MidiCCModulator if not present:
//   uint8_t stepSize = 10;            // CC values per step (1-127). Timing is automatic.
//   unsigned long lastStepTime = 0;   // timestamp of last step
//   bool stepperRampingDown = false;  // true when ramping back to 0 on release

// Timing model:
//   stepIntervalMs = rampUpTimeMs / (127 / stepSize)
//   This makes the staircase take the same total time as the smooth ramper.

inline void MidiCCModulator::updateStepper()
{
    // --- Ramp down to 0 on release (reuses ramper math) ---
    if (stepperRampingDown)
    {
        if (currentRampDuration <= 1)
        {
            currentValue = 0;
            isModulating = false;
            stepperRampingDown = false;
            sendMIDI(midiChannel, false, midiCCNumber, currentValue);
            return;
        }

        unsigned long now = millis();
        unsigned long elapsed = now - rampStartTime;

        if (elapsed >= currentRampDuration)
        {
            currentValue = 0;
            isModulating = false;
            stepperRampingDown = false;
            sendMIDI(midiChannel, false, midiCCNumber, currentValue);
            return;
        }

        float t = (float)elapsed / (float)currentRampDuration;
        if (t > 1.0f)
            t = 1.0f;

        float shaped = isLinear ? t : (t * t);
        int delta = (int)targetValue - (int)rampStartValue;
        uint8_t newVal = rampStartValue + (int8_t)(delta * shaped);

        if (newVal != currentValue)
        {
            currentValue = newVal;
            sendMIDI(midiChannel, false, midiCCNumber, currentValue);
        }
        return;
    }

    // --- Stopped at 127 or deactivated ---
    if (!isActivated)
    {
        isModulating = false;
        return;
    }

    // Already at ceiling — hold until released
    if (currentValue >= 127)
        return;

    // --- Step upward ---
    // Calculate interval so the full 0->127 journey takes rampUpTimeMs
    uint8_t clampedStep = (stepSize < 1) ? 1 : stepSize;
    float numSteps = 127.0f / (float)clampedStep;
    unsigned long stepInterval = (unsigned long)((float)rampUpTimeMs / numSteps);
    if (stepInterval < 1)
        stepInterval = 1;

    unsigned long now = millis();
    if (now - lastStepTime < stepInterval)
        return;

    lastStepTime = now;

    uint8_t newVal = currentValue + clampedStep;
    if (newVal > 127 || newVal < currentValue) // overflow guard
        newVal = 127;

    currentValue = newVal;
    sendMIDI(midiChannel, false, midiCCNumber, currentValue);
}
