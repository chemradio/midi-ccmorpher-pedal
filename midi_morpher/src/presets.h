#pragma once
// Preset button handling and LED update functions.
// Included after statePersistance.h, pedalState.h, and display.h in the .ino.

// ── Preset LED blink state ────────────────────────────────────────────────────
// Slow blink on the active preset LED when presetDirty signals unsaved changes.
// Fast blink burst on save to confirm the write.
#define PRESET_DIRTY_BLINK_MS 700    // full period (on + off)
#define PRESET_SAVE_BLINK_MS   80    // full period during save confirmation
#define PRESET_SAVE_BLINK_DUR 800    // total duration of save confirmation burst

inline unsigned long presetSaveBlinkUntil = 0;

inline void triggerPresetSaveBlink() {
    presetSaveBlinkUntil = millis() + PRESET_SAVE_BLINK_DUR;
}

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
                    triggerPresetSaveBlink();
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

// Drive the 6 preset LED GPIOs. Only the active preset's LED is lit.
// States applied to the active LED:
//   - Fast blink burst  → recent save (confirmation)
//   - Slow blink        → presetDirty (unsaved changes)
//   - Solid on          → clean / saved
inline void updatePresetLEDs(const PedalState &pedal) {
    unsigned long now = millis();
    bool activeOn = true;
    if(now < presetSaveBlinkUntil) {
        activeOn = ((now / (PRESET_SAVE_BLINK_MS / 2)) & 1) == 0;
    } else if(presetDirty) {
        activeOn = ((now / (PRESET_DIRTY_BLINK_MS / 2)) & 1) == 0;
    }
    for(int i = 0; i < 6; i++) {
        bool on = (i == (int)activePreset) && activeOn;
        digitalWrite(pedal.buttons[i].ledPin, on ? HIGH : LOW);
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
