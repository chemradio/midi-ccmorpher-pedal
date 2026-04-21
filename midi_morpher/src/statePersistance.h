#pragma once
#include <Preferences.h>

// ── NVS handle ────────────────────────────────────────────────────────────────
inline Preferences prefs;

// ── Persisted button record ────────────────────────────────────────────────────
// Layout (16 bytes, same as before ccLow/ccHigh were added):
//   [0] modeIndex  [1] midiNumber  [2-3] padding  [4-7] rampUpMs  [8-11] rampDownMs
//   [12] fsChannel  [13] ccLow  [14] ccHigh  [15] _pad
// When loading old NVS data: bytes 13-15 were implicit padding (zero-filled by
// aggregate-init). ccHigh==0 is treated as 127 in applyPreset (upgrade path).
struct FSButtonPersisted {
    uint8_t  modeIndex;
    uint8_t  midiNumber;
    uint32_t rampUpMs;
    uint32_t rampDownMs;
    uint8_t  fsChannel;  // 0xFF = follow global; 0–15 = per-FS override; keyboard: modifier bitmask
    uint8_t  ccLow;      // value sent on CC release / latch-off (default 0)
    uint8_t  ccHigh;     // value sent on CC press / latch-on (default 127; 0 = treat as 127)
    uint8_t  _pad;       // keeps sizeof == 16
};

// ── Preset data ───────────────────────────────────────────────────────────────
struct PresetData {
    uint8_t midiChannel;
    FSButtonPersisted buttons[6];
    float   bpm;   // per-preset BPM for the internal clock (DEFAULT_BPM on first boot)
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
    p.bpm         = midiClock.bpm;
    for(int i = 0; i < 6; i++) {
        p.buttons[i] = {
            state.buttons[i].modeIndex,
            state.buttons[i].midiNumber,
            (uint32_t)state.buttons[i].rampUpMs,
            (uint32_t)state.buttons[i].rampDownMs,
            state.buttons[i].fsChannel,
            state.buttons[i].ccLow,
            state.buttons[i].ccHigh,
            0  // _pad
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
    midiClock.setBpm(p.bpm > 0 ? p.bpm : DEFAULT_BPM);
    for(int i = 0; i < 6; i++) {
        FSButton &btn    = state.buttons[i];
        btn.midiNumber   = p.buttons[i].midiNumber;
        btn.rampUpMs     = p.buttons[i].rampUpMs;
        btn.rampDownMs   = p.buttons[i].rampDownMs;
        btn.fsChannel    = p.buttons[i].fsChannel;
        btn.ccLow        = p.buttons[i].ccLow;
        // ccHigh==0 means old NVS padding byte — treat as default 127
        btn.ccHigh       = (p.buttons[i].ccHigh == 0) ? 127 : p.buttons[i].ccHigh;
        uint8_t mi       = p.buttons[i].modeIndex < NUM_MODES ? p.buttons[i].modeIndex : 0;
        applyModeFlags(btn, mi);
    }
    // Reset all per-FS modulators to a clean resting state.
    for(int i = 0; i < 6; i++) {
        state.modulators[i].restingHigh = false;
        state.modulators[i].reset();
    }
    activePreset = idx;
    presetDirty  = false;
}

// ── Global settings (non-preset) ─────────────────────────────────────────────

inline void saveGlobalSettings(const PedalState &state) {
    prefs.begin("globals", false);
    prefs.putBytes("gs", &state.globalSettings, sizeof(GlobalSettings));
    prefs.end();
}

inline void loadGlobalSettings(PedalState &state) {
    prefs.begin("globals", true);
    size_t sz = prefs.getBytesLength("gs");
    bool loaded = (sz == sizeof(GlobalSettings));
    if(loaded) prefs.getBytes("gs", &state.globalSettings, sizeof(GlobalSettings));
    prefs.end();
    if(!loaded) {
        state.globalSettings = GlobalSettings{};  // struct defaults
        saveGlobalSettings(state);
    }
}

// ── Load all presets from NVS on boot. ───────────────────────────────────────
// Handles three cases:
//   1. Current format (sizeof presets): load directly.
//   2. Legacy format without the bpm field: copy, set bpm = DEFAULT_BPM, upgrade NVS.
//   3. Unknown / first boot: factory defaults, optionally migrate old "pedal" namespace.
inline void loadAllPresets(PedalState &state) {
    // Matches the old PresetData layout (before bpm was added).
    struct LegacyPresetData { uint8_t midiChannel; FSButtonPersisted buttons[6]; };

    prefs.begin("presets", true);
    size_t sz       = prefs.getBytesLength("prs");
    bool   loaded   = (sz == sizeof(presets));
    bool   isLegacy = (!loaded && sz == (size_t)NUM_PRESETS * sizeof(LegacyPresetData));

    if(loaded) {
        prefs.getBytes("prs", presets, sizeof(presets));
        activePreset = constrain(prefs.getUChar("act", 0), 0, NUM_PRESETS - 1);
    } else if(isLegacy) {
        LegacyPresetData old[NUM_PRESETS];
        prefs.getBytes("prs", old, sz);
        activePreset = constrain(prefs.getUChar("act", 0), 0, NUM_PRESETS - 1);
        for(int p = 0; p < NUM_PRESETS; p++) {
            presets[p].midiChannel = old[p].midiChannel;
            presets[p].bpm         = DEFAULT_BPM;
            for(int i = 0; i < 6; i++) presets[p].buttons[i] = old[p].buttons[i];
        }
    }
    prefs.end();

    if(!loaded && !isLegacy) {
        // Factory defaults — all presets identical.
        for(int p = 0; p < NUM_PRESETS; p++) {
            presets[p].midiChannel = 0;
            presets[p].bpm         = DEFAULT_BPM;
            for(int i = 0; i < 6; i++)
                presets[p].buttons[i] = {0, 0, DEFAULT_RAMP_SPEED, DEFAULT_RAMP_SPEED, 0xFF, 0, 127, 0};
        }
        // Migrate old single-preset "pedal" namespace into preset 0 if it exists.
        prefs.begin("pedal", true);
        if(prefs.getBytesLength("cfg") == sizeof(LegacyPresetData)) {
            LegacyPresetData old;
            prefs.getBytes("cfg", &old, sizeof(old));
            presets[0].midiChannel = old.midiChannel;
            presets[0].bpm         = DEFAULT_BPM;
            for(int i = 0; i < 6; i++) presets[0].buttons[i] = old.buttons[i];
        }
        prefs.end();
        activePreset = 0;
    }

    if(!loaded) saveAllPresets();   // write upgraded / default data back

    applyPreset(activePreset, state);
}
