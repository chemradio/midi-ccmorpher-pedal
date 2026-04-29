#pragma once
#include <Preferences.h>
#include <LittleFS.h>

// Preset file layout version. Bumped whenever PresetData / FSButtonPersisted
// layouts change. Magic + version sit at the start of /presets.bin; mismatched
// files are discarded and replaced with factory defaults — no silent garbage.
static constexpr uint32_t PRESET_FILE_MAGIC   = 0x4D4D5031UL; // "MMP1"
static constexpr uint32_t PRESET_FILE_VERSION = 1;
static constexpr const char *PRESET_FILE_PATH = "/presets.bin";

struct PresetFileHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t presetCount;
    uint32_t recordSize;
};

// ── NVS handle ────────────────────────────────────────────────────────────────
inline Preferences prefs;

// FSActionPersisted is defined in sharedTypes.h so PedalState can carry a
// live copy of the active preset's load action without a cyclic include.

// ── Persisted button record (64 bytes) ────────────────────────────────────────
// Extra actions: 3 × FSActionPersisted (48 bytes). Size change resets presets.
struct FSButtonPersisted {
    uint8_t  modeIndex;
    uint8_t  midiNumber;
    uint32_t rampUpMs;
    uint32_t rampDownMs;
    uint8_t  fsChannel;  // 0xFF = follow global; keyboard: modifier bitmask
    uint8_t  ccLow;
    uint8_t  ccHigh;
    uint8_t  velocity;   // Note On velocity (replaces _pad)
    FSActionPersisted extraActions[FS_NUM_EXTRA];  // LONG, DOUBLE, RELEASE
};

// ── Preset data ───────────────────────────────────────────────────────────────
struct PresetData {
    uint8_t midiChannel;
    FSButtonPersisted buttons[6];
    float   bpm;            // per-preset BPM for the internal clock (DEFAULT_BPM on first boot)
    FSActionPersisted loadAction;  // optional action fired when this preset is loaded (enabled=0 → none)
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
    File f = LittleFS.open(PRESET_FILE_PATH, "w");
    if(f) {
        PresetFileHeader hdr = { PRESET_FILE_MAGIC, PRESET_FILE_VERSION,
                                 (uint32_t)NUM_PRESETS, (uint32_t)sizeof(PresetData) };
        f.write((const uint8_t*)&hdr, sizeof(hdr));
        f.write((const uint8_t*)presets, sizeof(presets));
        f.close();
    }
    prefs.begin("presets", false);
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
        pb.velocity   = btn.velocity;
        for(int t = 0; t < (int)FS_NUM_EXTRA; t++) {
            const FSAction &act = btn.extraActions[t];
            FSActionPersisted &ap = pb.extraActions[t];
            ap.enabled    = act.enabled ? 1u : 0u;
            ap.modeIndex  = act.modeIndex;
            ap.midiNumber = act.midiNumber;
            ap.fsChannel  = act.fsChannel;
            ap.ccLow      = act.ccLow;
            ap.ccHigh     = act.ccHigh;
            ap.velocity   = act.velocity;
            ap._pad       = 0;
            ap.rampUpMs   = (uint32_t)act.rampUpMs;
            ap.rampDownMs = (uint32_t)act.rampDownMs;
        }
    }
    // Flush the live load-action edit into the persisted slot before writing.
    p.loadAction = state.liveLoadAction;
    saveAllPresets();
    presetDirty = false;
}

// Fire a preset load action (if enabled) using a temporary FSButton.
// Uses modulators[0] for modulation modes.
inline void _fireLoadAction(const FSActionPersisted &act, PedalState &state) {
    if(!act.enabled || act.modeIndex >= NUM_MODES) return;
    FSButton tmp(255, 255, "", 0);
    applyModeFlags(tmp, act.modeIndex);
    tmp.midiNumber = act.midiNumber;
    tmp.fsChannel  = act.fsChannel;
    tmp.ccLow      = act.ccLow;
    tmp.ccHigh     = (act.ccHigh == 0) ? 127 : act.ccHigh;
    tmp.velocity   = (act.velocity == 0) ? 100 : act.velocity;
    tmp.rampUpMs   = act.rampUpMs;
    tmp.rampDownMs = act.rampDownMs;
    uint8_t ch = (tmp.fsChannel == 0xFF) ? state.midiChannel : tmp.fsChannel;
    tmp._applyPressState(true, ch, state.modulators[0], nullptr);
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
        btn.velocity     = (pb.velocity == 0) ? 100 : pb.velocity;
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
            act.velocity   = (ap.velocity == 0) ? 100 : ap.velocity;
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
    // Seed the live load-action editor from the freshly-loaded slot. Any
    // unsaved edits to the previous preset's live copy are simply dropped —
    // same model the rest of pedal state uses.
    state.liveLoadAction = p.loadAction;
    activePreset = idx;
    presetDirty  = false;
    _fireLoadAction(p.loadAction, state);
}

// ── Global settings (non-preset) ─────────────────────────────────────────────

inline void saveGlobalSettings(const PedalState &state) {
    prefs.begin("globals", false);
    prefs.putBytes("gs", &state.globalSettings, sizeof(GlobalSettings));
    prefs.end();
}

inline void loadGlobalSettings(PedalState &state) {
    state.globalSettings = GlobalSettings{};  // struct defaults first
    prefs.begin("globals", true);
    size_t sz = prefs.getBytesLength("gs");
    if(sz > 0 && sz <= sizeof(GlobalSettings))
        prefs.getBytes("gs", &state.globalSettings, sz);
    prefs.end();
    if(sz != sizeof(GlobalSettings))
        saveGlobalSettings(state);
}

// ── Load all presets from LittleFS on boot. ──────────────────────────────────
// Reads /presets.bin if present, validates magic + version + sizes; on any
// mismatch falls through to factory defaults rather than loading garbage.
inline void loadAllPresets(PedalState &state) {
    bool loaded = false;
    if(LittleFS.exists(PRESET_FILE_PATH)) {
        File f = LittleFS.open(PRESET_FILE_PATH, "r");
        if(f && (size_t)f.size() == sizeof(PresetFileHeader) + sizeof(presets)) {
            PresetFileHeader hdr{};
            if(f.read((uint8_t*)&hdr, sizeof(hdr)) == sizeof(hdr) &&
               hdr.magic       == PRESET_FILE_MAGIC &&
               hdr.version     == PRESET_FILE_VERSION &&
               hdr.presetCount == (uint32_t)NUM_PRESETS &&
               hdr.recordSize  == (uint32_t)sizeof(PresetData)) {
                f.read((uint8_t*)presets, sizeof(presets));
                loaded = true;
            }
        }
        if(f) f.close();
    }
    if(!loaded) {
        for(int p = 0; p < NUM_PRESETS; p++) {
            presets[p].midiChannel = 0;
            presets[p].bpm         = DEFAULT_BPM;
            for(int i = 0; i < 6; i++)
                presets[p].buttons[i] = {1, 0, DEFAULT_RAMP_SPEED, DEFAULT_RAMP_SPEED, 0xFF, 0, 127, 100};
        }
        saveAllPresets();
    }
    prefs.begin("presets", true);
    activePreset = constrain(prefs.getUChar("act", 0), 0, NUM_PRESETS - 1);
    prefs.end();
    // applyPreset seeds state.liveLoadAction from the chosen slot.
    applyPreset(activePreset, state);
}
