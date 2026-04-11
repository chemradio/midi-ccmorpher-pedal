#pragma once
#include <Preferences.h>

// ── NVS handle ────────────────────────────────────────────────────────────────
inline Preferences prefs;

// ── Persisted button record ────────────────────────────────────────────────────
struct FSButtonPersisted {
    uint8_t  modeIndex;
    uint8_t  midiNumber;
    uint32_t rampUpMs;
    uint32_t rampDownMs;
    uint8_t  fsChannel;  // 0xFF = follow global; 0–15 = per-FS override
};

// ── Preset data ───────────────────────────────────────────────────────────────
struct PresetData {
    uint8_t midiChannel;
    FSButtonPersisted buttons[6];
};

inline PresetData presets[NUM_PRESETS];
inline uint8_t   activePreset = 0;
inline bool      presetDirty  = false;

// ── Dirty flag — replaces auto-save; callers unchanged ────────────────────────
inline void markStateDirty() {
    presetDirty = true;
}

// ── NVS helpers ───────────────────────────────────────────────────────────────

inline void saveAllPresets() {
    prefs.begin("presets", false);
    prefs.putBytes("prs", presets, sizeof(presets));
    prefs.putUChar("act", activePreset);
    prefs.end();
}

// Save current live state into the active preset slot and flush to NVS.
// Defined after PedalState is available (included before this header in .ino).
inline void saveCurrentPreset(const PedalState &state) {
    PresetData &p = presets[activePreset];
    p.midiChannel = state.midiChannel;
    for(int i = 0; i < 6; i++) {
        p.buttons[i] = {
            state.buttons[i].modeIndex,
            state.buttons[i].midiNumber,
            (uint32_t)state.buttons[i].rampUpMs,
            (uint32_t)state.buttons[i].rampDownMs,
            state.buttons[i].fsChannel
        };
    }
    saveAllPresets();
    presetDirty = false;
}

// Apply a stored preset to the live pedal state.
// applyModeFlags() is available because footswitchObject.h is included first.
inline void applyPreset(uint8_t idx, PedalState &state) {
    if(idx >= NUM_PRESETS) idx = 0;
    const PresetData &p = presets[idx];
    state.setMidiChannel(constrain(p.midiChannel, 0, 15));
    for(int i = 0; i < 6; i++) {
        FSButton &btn    = state.buttons[i];
        btn.midiNumber   = p.buttons[i].midiNumber;
        btn.rampUpMs     = p.buttons[i].rampUpMs;
        btn.rampDownMs   = p.buttons[i].rampDownMs;
        btn.fsChannel    = p.buttons[i].fsChannel;
        uint8_t mi       = p.buttons[i].modeIndex < NUM_MODES ? p.buttons[i].modeIndex : 0;
        applyModeFlags(btn, mi);
    }
    // Reset modulator to a clean resting state.
    state.modulator.restingHigh = false;
    state.modulator.reset();
    activePreset = idx;
    presetDirty  = false;
}

// Load all presets from NVS on boot.
// Migrates from old "pedal" namespace (same struct layout) into preset 0 if found.
inline void loadAllPresets(PedalState &state) {
    prefs.begin("presets", true);
    size_t sz     = prefs.getBytesLength("prs");
    bool   loaded = (sz == sizeof(presets));
    if(loaded) {
        prefs.getBytes("prs", presets, sizeof(presets));
        activePreset = constrain(prefs.getUChar("act", 0), 0, NUM_PRESETS - 1);
    }
    prefs.end();

    if(!loaded) {
        // Factory defaults — all presets identical.
        for(int p = 0; p < NUM_PRESETS; p++) {
            presets[p].midiChannel = 0;
            for(int i = 0; i < 6; i++) {
                presets[p].buttons[i] = {0, 0, DEFAULT_RAMP_SPEED, DEFAULT_RAMP_SPEED, 0xFF};
            }
        }
        // Migrate old "pedal" namespace into preset 0 if it exists.
        // PresetData and the old PedalPersisted struct share identical layout (67 bytes).
        prefs.begin("pedal", true);
        if(prefs.getBytesLength("cfg") == sizeof(PresetData)) {
            prefs.getBytes("cfg", &presets[0], sizeof(PresetData));
        }
        prefs.end();

        activePreset = 0;
        saveAllPresets();
    }

    applyPreset(activePreset, state);
}
