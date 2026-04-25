#pragma once
#include <Preferences.h>

// ── NVS handle ────────────────────────────────────────────────────────────────
inline Preferences prefs;

// ── Persisted extra action (16 bytes) ─────────────────────────────────────────
struct FSActionPersisted {
    uint8_t  enabled;    // 0 = disabled
    uint8_t  modeIndex;
    uint8_t  midiNumber;
    uint8_t  fsChannel;
    uint8_t  ccLow;
    uint8_t  ccHigh;
    uint8_t  _pad[2];    // align rampUpMs to 4 bytes
    uint32_t rampUpMs;
    uint32_t rampDownMs;
};  // 16 bytes

// ── Persisted button record (64 bytes) ────────────────────────────────────────
// Primary action: first 16 bytes (legacy layout preserved).
// Extra actions: 3 × FSActionPersisted (48 bytes). NVS size change resets presets.
struct FSButtonPersisted {
    uint8_t  modeIndex;
    uint8_t  midiNumber;
    uint32_t rampUpMs;
    uint32_t rampDownMs;
    uint8_t  fsChannel;  // 0xFF = follow global; 0–15 = per-FS override; keyboard: modifier bitmask
    uint8_t  ccLow;      // value sent on CC release / latch-off (default 0)
    uint8_t  ccHigh;     // value sent on CC press / latch-on (default 127; 0 = treat as 127)
    uint8_t  _pad;
    FSActionPersisted extraActions[FS_NUM_EXTRA];  // LONG, DOUBLE, RELEASE
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
        const FSButton &btn = state.buttons[i];
        FSButtonPersisted &pb = p.buttons[i];
        pb.modeIndex  = btn.modeIndex;
        pb.midiNumber = btn.midiNumber;
        pb.rampUpMs   = (uint32_t)btn.rampUpMs;
        pb.rampDownMs = (uint32_t)btn.rampDownMs;
        pb.fsChannel  = btn.fsChannel;
        pb.ccLow      = btn.ccLow;
        pb.ccHigh     = btn.ccHigh;
        pb._pad       = 0;
        for(int t = 0; t < (int)FS_NUM_EXTRA; t++) {
            const FSAction &act = btn.extraActions[t];
            FSActionPersisted &ap = pb.extraActions[t];
            ap.enabled    = act.enabled ? 1u : 0u;
            ap.modeIndex  = act.modeIndex;
            ap.midiNumber = act.midiNumber;
            ap.fsChannel  = act.fsChannel;
            ap.ccLow      = act.ccLow;
            ap.ccHigh     = act.ccHigh;
            ap._pad[0]    = 0;
            ap._pad[1]    = 0;
            ap.rampUpMs   = (uint32_t)act.rampUpMs;
            ap.rampDownMs = (uint32_t)act.rampDownMs;
        }
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
        const FSButtonPersisted &pb = p.buttons[i];
        btn.midiNumber   = pb.midiNumber;
        btn.rampUpMs     = pb.rampUpMs;
        btn.rampDownMs   = pb.rampDownMs;
        btn.fsChannel    = pb.fsChannel;
        btn.ccLow        = pb.ccLow;
        btn.ccHigh       = (pb.ccHigh == 0) ? 127 : pb.ccHigh;
        uint8_t mi       = pb.modeIndex < NUM_MODES ? pb.modeIndex : 0;
        applyModeFlags(btn, mi);
        // Load extra actions
        for(int t = 0; t < (int)FS_NUM_EXTRA; t++) {
            const FSActionPersisted &ap = pb.extraActions[t];
            FSAction &act = btn.extraActions[t];
            act.enabled    = ap.enabled != 0;
            act.modeIndex  = ap.modeIndex < NUM_MODES ? ap.modeIndex : 0;
            act.midiNumber = ap.midiNumber;
            act.fsChannel  = ap.fsChannel;
            act.ccLow      = ap.ccLow;
            act.ccHigh     = ap.ccHigh != 0 ? ap.ccHigh : 127;
            act.rampUpMs   = ap.rampUpMs;
            act.rampDownMs = ap.rampDownMs;
        }
        // Reset press state machine
        btn.pressPhase  = PressPhase::IDLE;
        btn.longFired   = false;
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
                presets[p].buttons[i] = {1, 0, DEFAULT_RAMP_SPEED, DEFAULT_RAMP_SPEED, 0xFF, 0, 127, 0}; // modeIndex=1=PC
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
