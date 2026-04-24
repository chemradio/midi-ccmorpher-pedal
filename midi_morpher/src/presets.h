#pragma once
// Preset button handling and LED update functions.
// Included after statePersistance.h, pedalState.h, and display.h in the .ino.

// ── Preset LED blink state ────────────────────────────────────────────────────
// Slow blink on the active preset LED when presetDirty signals unsaved changes
// (except in conservative mode).
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
//   - Slow blink        → presetDirty (unsaved changes; non-conservative only)
//   - Conservative mode → blink on preset select/save activity, fully off at idle
//   - On mode           → full on when not blinking
inline void updatePresetLEDs(const PedalState &pedal) {
    uint8_t mode = pedal.globalSettings.ledMode;

    if(mode == LED_MODE_OFF) {
        for(int i = 0; i < 6; i++) analogWrite(pedal.buttons[i].ledPin, 0);
        return;
    }

    unsigned long now = millis();

    // Conservative: track preset changes and trigger an activity blink on load.
    static uint8_t       conservativeLastPreset = 0xFF;
    static unsigned long conservativeBlinkUntil = 0;
    if(activePreset != conservativeLastPreset) {
        conservativeLastPreset = activePreset;
        conservativeBlinkUntil = now + CONSERVATIVE_ACTIVITY_BLINK_DUR;
    }

    bool activeOn = true;
    if(now < presetSaveBlinkUntil) {
        activeOn = ((now / (PRESET_SAVE_BLINK_MS / 2)) & 1) == 0;
    } else if(mode == LED_MODE_CONSERVATIVE && now < conservativeBlinkUntil) {
        activeOn = ((now / (CONSERVATIVE_ACTIVITY_BLINK_MS / 2)) & 1) == 0;
    } else if(mode != LED_MODE_CONSERVATIVE && presetDirty) {
        activeOn = ((now / (PRESET_DIRTY_BLINK_MS / 2)) & 1) == 0;
    } else if(mode == LED_MODE_CONSERVATIVE) {
        activeOn = false; // fully off when idle in conservative mode
    }

    for(int i = 0; i < 6; i++) {
        if(i != (int)activePreset || !activeOn) {
            analogWrite(pedal.buttons[i].ledPin, 0);
            continue;
        }
        analogWrite(pedal.buttons[i].ledPin, 255);
    }
}

// Drive the activity LED: reflects the logical ledState of the last-pressed footswitch.
inline void updateActivityLed(const PedalState &pedal) {
    if(pedal.globalSettings.ledMode == LED_MODE_OFF) {
        digitalWrite(ACTIVITY_LED_PIN, LOW);
        return;
    }
    bool active = false;
    if(pedal.lastActiveFSIndex >= 0)
        active = pedal.buttons[pedal.lastActiveFSIndex].ledState;
    digitalWrite(ACTIVITY_LED_PIN, active ? HIGH : LOW);
}

// ── Combined footswitch actions for preset navigation ───────────────────────────
// FS1 + FS2 simultaneous press → preset down (previous preset)
// FS3 + FS4 simultaneous press → preset up (next preset)
// FS1 + FS4 held → save preset (long-press, like preset button long-press)


inline void handleCombinedActions(PedalState &pedal) {
    static uint8_t prevCombinedState = 0;
    static unsigned long lastCombinedMs = 0;
    static unsigned long combinedHoldStart = 0;
    static bool combinedHoldFired = false;

    uint8_t combined = 0;
    if(pedal.buttons[0].isPressed && pedal.buttons[1].isPressed) combined |= 0x01;
    if(pedal.buttons[2].isPressed && pedal.buttons[3].isPressed) combined |= 0x02;
    if(pedal.buttons[0].isPressed && pedal.buttons[3].isPressed) combined |= 0x04;

    unsigned long now = millis();
    if(now - lastCombinedMs < 300) {
        if((combined & 0x04) && !(prevCombinedState & 0x04)) {
            combinedHoldStart = now;
            combinedHoldFired = false;
        } else if((prevCombinedState & 0x04) && !(combined & 0x04)) {
            combinedHoldFired = false;
        }
        prevCombinedState = combined;
        return;
    }

    if((combined & 0x01) && !(prevCombinedState & 0x01)) {
        uint8_t next = (activePreset == 0) ? (NUM_PRESETS - 1) : (activePreset - 1);
        applyPreset(next, pedal);
        displayPresetLoad(activePreset);
        lastCombinedMs = now;
    } else if((combined & 0x02) && !(prevCombinedState & 0x02)) {
        uint8_t next = (activePreset + 1) % NUM_PRESETS;
        applyPreset(next, pedal);
        displayPresetLoad(activePreset);
        lastCombinedMs = now;
    } else if((combined & 0x04) && !(prevCombinedState & 0x04)) {
        combinedHoldStart = now;
        combinedHoldFired = false;
    } else if((prevCombinedState & 0x04) && !(combined & 0x04)) {
        combinedHoldFired = false;
    }

    if((combined & 0x04) && !combinedHoldFired && (now - combinedHoldStart) >= FS_LONG_MS) {
        if(!pedal.settingsLocked) {
            saveCurrentPreset(pedal);
            triggerPresetSaveBlink();
            displayPresetSaved(activePreset);
        }
        combinedHoldFired = true;
        lastCombinedMs = now;
    }

    prevCombinedState = combined;
}
