#pragma once
// Preset button handling and LED update functions.
// Included after statePersistance.h, pedalState.h, and display.h in the .ino.

// ── Preset button (GPIO PRESET_BTN_PIN) ───────────────────────────────────────
// Short press: cycle to next preset.
// Long press (≥ PRESET_SAVE_HOLD_MS): save current state to active preset.
// Loading works even when LOCK is engaged; saving is blocked when locked.

inline void handlePresetButton(PedalState &pedal) {
    static unsigned long pressStart  = 0;
    static bool          lastBtnState  = HIGH;
    static bool          longPressFired = false;

    bool reading = digitalRead(PRESET_BTN_PIN);

    if(reading == lastBtnState) {
        // Check for long-press threshold while held.
        if(reading == LOW && pressStart > 0 && !longPressFired) {
            if(millis() - pressStart >= PRESET_SAVE_HOLD_MS) {
                longPressFired = true;
                if(!pedal.settingsLocked) {
                    saveCurrentPreset(pedal);
                    displayPresetSaved(activePreset);
                }
            }
        }
        return;
    }

    lastBtnState = reading;

    if(reading == LOW) {
        // Button just pressed — start timer.
        pressStart     = millis();
        longPressFired = false;
    } else {
        // Button released — if no long press fired, do a short-press cycle.
        if(!longPressFired) {
            uint8_t next = (activePreset + 1) % NUM_PRESETS;
            applyPreset(next, pedal);
            displayPresetLoad(activePreset);
        }
        pressStart = 0;
    }
}

// ── LED output functions ───────────────────────────────────────────────────────

// Drive the 6 footswitch LED GPIOs: only the active preset's LED is HIGH.
inline void updatePresetLEDs(const PedalState &pedal) {
    for(int i = 0; i < 6; i++) {
        digitalWrite(pedal.buttons[i].ledPin, (i == (int)activePreset) ? HIGH : LOW);
    }
}

// Drive the activity LED: reflects the logical ledState of the last-pressed footswitch.
inline void updateActivityLed(const PedalState &pedal) {
    bool active = false;
    if(pedal.lastActiveFSIndex >= 0) {
        active = pedal.buttons[pedal.lastActiveFSIndex].ledState;
    }
    digitalWrite(ACTIVITY_LED_PIN, active ? HIGH : LOW);
}
