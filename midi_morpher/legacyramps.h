#pragma once

// pseudo
direction switch deprecated.to switch the initial direction of ramp user must toggle latching hotswitch and ramp up and disable latching on the hotswitch.

    uint8_t currentValue = 0;

void setup()
{
    sendMIDI(currentValue);
}

void loop()
{
    // Read the direction switch to update the preferred starting direction
    // This only affects which direction we ramp when the footswitch is pressed
    // It does NOT reset the current value
    rampDirectionUp = (digitalRead(DIRECTION_SWITCH_PIN) == HIGH);

    // Read footswitch with debounce
    bool reading = digitalRead(FOOTSWITCH_PIN) == LOW; // Active low (pressed = LOW)

    if (reading != lastFootswitchState)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (reading != footswitchState)
        {
            footswitchState = reading;
            handleFootswitch(footswitchState);
        }
    }

    lastFootswitchState = reading;

    // Process ramping
    if (isRamping)
    {
        updateRamp();
    }
}

void handleFootswitch(bool pressed)
{
    bool isMomentaryMode = digitalRead(MODE_SWITCH_PIN) == HIGH;

    if (isMomentaryMode)
    {
        // Momentary mode: press starts ramp, release reverses
        if (pressed)
        {
            startRamp(rampDirectionUp);
        }
        else
        {
            // Reverse direction from current position
            startRamp(!rampDirectionUp);
        }
    }
    else
    {
        // Latching mode: each press toggles
        if (pressed)
        {
            latchState = !latchState;
            startRamp(latchState ? rampDirectionUp : !rampDirectionUp);
        }
    }
}

void startRamp(bool goingUp)
{
    // FIX: Store the actual starting value for this ramp
    rampStartValue = currentValue;

    // Determine target and base duration
    if (goingUp)
    {
        targetValue = 127;
        rampDuration = map(analogRead(RAMP_UP_POT_PIN), 0, 1023, minRampDuration, maxRampDuration);
    }
    else
    {
        targetValue = 0;
        rampDuration = map(analogRead(RAMP_DOWN_POT_PIN), 0, 1023, minRampDuration, maxRampDuration);
    }

    // FIX: Calculate proportional duration based on actual distance to travel
    int distance = abs(targetValue - rampStartValue);
    if (distance < 127)
    {
        rampDuration = (rampDuration * distance) / 127;
    }

    // Ensure minimum duration
    if (rampDuration < 100)
        rampDuration = 100;

    isRamping = true;
    lastRampTime = millis();
    rampDirectionUp = goingUp;

    // Send immediate MIDI message at the start of the ramp for instant feedback
    sendMIDI(currentValue);
    lastMidiSendTime = millis();
}

void updateRamp()
{
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - lastRampTime;

    if (elapsed >= rampDuration)
    {
        // Ramp complete
        currentValue = targetValue;
        isRamping = false;
        sendMIDI(currentValue);
        return;
    }

    // Only update at the specified interval to avoid MIDI flooding
    if (currentTime - lastMidiSendTime < midiSendInterval)
    {
        return; // Skip this update, too soon
    }

    // Calculate progress (0.0 to 1.0)
    float rampProgress = (float)elapsed / (float)rampDuration;

    // Apply curve
    float curvedProgress = rampProgress;
    bool isLinear = digitalRead(CURVE_SWITCH_PIN) == HIGH;

    if (!isLinear)
    {
        // Exponential curve (ease-in)
        curvedProgress = curvedProgress * curvedProgress;
    }

    // FIX: Calculate current value using the STORED start value and target
    // Simple formula: current = start + (distance * progress)
    int newValue = rampStartValue + (int)((targetValue - rampStartValue) * curvedProgress);
    newValue = constrain(newValue, 0, 127);

    if (newValue != currentValue)
    {
        currentValue = newValue;
        sendMIDI(currentValue);
        lastMidiSendTime = currentTime; // Record when we sent MIDI
    }
}
