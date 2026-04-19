#pragma once
#include "../config.h"
#include "../clock/midiClock.h"
#include "../midiCCModulator.h"
#include "../midiOut.h"
#include "../sharedTypes.h"

// ── Footswitch mode enum ─────────────────────────────────────────────────────
// Order matches the spec mode list exactly (26 modes).

enum class FootswitchMode {
  // Basic
  MomentaryPC,
  MomentaryCC,
  LatchingCC,
  MomentaryNote,

  // Ramper
  RamperMomentary,
  RamperLatching,
  RamperInvMomentary,
  RamperInvLatching,

  // LFO — three wave shapes × momentary/latching
  LfoSineMomentary,
  LfoSineLatching,
  LfoTriMomentary,
  LfoTriLatching,
  LfoSqMomentary,
  LfoSqLatching,

  // Stepper
  StepperMomentary,
  StepperLatching,
  StepperInvMomentary,
  StepperInvLatching,

  // Random Stepper
  RandomMomentary,
  RandomLatching,
  RandomInvMomentary,
  RandomInvLatching,

  // Scene / Snapshot — each scene is followed by its scroll variant
  HelixSnapshot,
  HelixSnapshotScroll,
  QuadCortexScene,
  QuadCortexSceneScroll,
  FractalScene,
  FractalSceneScroll,
  KemperSlot,
  KemperSlotScroll,

  // Clock control
  TapTempo,

  // System / Transport MIDI commands — encoder selects from systemCommands[]
  System,
};

// ── ModeInfo ─────────────────────────────────────────────────────────────────

struct ModeInfo {
  FootswitchMode mode;
  bool isLatching;
  bool isPC;
  bool isNote;
  bool isModSwitch;
  bool isInverted;   // resting position is 127
  bool isScene;      // Helix / QC / Fractal / Kemper
  const char *name;
  ModulationType modType;
  RampShape shape;     // ramp/wave shape applied to this mode
  uint8_t sceneCC;     // base CC number for scene/snapshot modes
  uint8_t sceneMaxVal; // encoder ceiling (7 for 0–7, 4 for Kemper slots 0–4)
  bool scenePickCC;    // true = encoder selects CC number (Kemper), false = encoder selects value
  bool isSceneScroll;  // true = each press advances through 0..midiNumber then wraps
  bool isSystem;       // true = System/Transport mode; midiNumber indexes systemCommands[]
};

// ── System / Transport command table ────────────────────────────────────────
// midiNumber in System mode is an index into this table. Order = priority
// (most-used first). Max encoder value = NUM_SYS_CMDS - 1.

enum SysCmdKind : uint8_t { SYS_MMC, SYS_SPP };

struct SysCmdInfo {
  const char *name;
  SysCmdKind  kind;
  uint8_t     data;  // MMC sub-id or SPP pos (always 0 here)
  uint8_t     rt;    // optional System Real-Time byte to also send (0 = none)
};

// Play/Stop/Continue emit BOTH MMC SysEx and the matching real-time byte so
// the pedal works with DAWs in "Listen to MMC" mode (Logic / Pro Tools / Cubase
// etc. — MMC triggers transport) AND with gear/DAWs slaved to external MIDI
// clock (hardware sequencers, drum machines — real-time bytes trigger transport).
// MMC receivers ignore the real-time byte and vice-versa, so dual-emit is safe.
inline constexpr SysCmdInfo systemCommands[] = {
  { "Play",      SYS_MMC, 0x02, 0xFA },  // MMC Play + Start
  { "Stop",      SYS_MMC, 0x01, 0xFC },  // MMC Stop + Stop RT
  { "Continue",  SYS_MMC, 0x03, 0xFB },  // MMC Deferred Play + Continue RT
  { "Record",    SYS_MMC, 0x06, 0x00 },  // MMC Record Strobe
  { "Pause",     SYS_MMC, 0x09, 0x00 },  // MMC Pause
  { "Rewind",    SYS_MMC, 0x05, 0x00 },  // MMC Rewind
  { "FFwd",      SYS_MMC, 0x04, 0x00 },  // MMC Fast Forward
  { "GotoStart", SYS_SPP, 0x00, 0x00 },  // Song Position 0
};
inline constexpr uint8_t NUM_SYS_CMDS = sizeof(systemCommands) / sizeof(systemCommands[0]);

inline void sendSystemCommand(uint8_t idx) {
  if(idx >= NUM_SYS_CMDS) return;
  const SysCmdInfo &c = systemCommands[idx];
  switch(c.kind) {
    case SYS_MMC: sendMMC(c.data);           break;
    case SYS_SPP: sendSPP((uint16_t)c.data); break;
  }
  if(c.rt) sendSystemRealtime(c.rt);
}

// ── Mode table ───────────────────────────────────────────────────────────────
// Single source of truth for all 26 modes.
// Columns: mode, isLatching, isPC, isNote, isModSwitch, isInverted, isScene,
//          name, modType, shape, sceneCC, sceneMaxVal, scenePickCC

inline constexpr ModeInfo modes[] = {
  // ── Basic ────────────────────────────────────────────────────────────────
  { FootswitchMode::MomentaryPC,        false, true,  false, false, false, false, "PC",            ModulationType::NOMODULATION, SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::MomentaryCC,        false, false, false, false, false, false, "CC",            ModulationType::NOMODULATION, SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::LatchingCC,         true,  false, false, false, false, false, "CC Latch",      ModulationType::NOMODULATION, SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::MomentaryNote,      false, false, true,  false, false, false, "Note",          ModulationType::NOMODULATION, SHAPE_LINEAR, 0,  0, false },

  // ── Ramper — exponential curve per spec ───────────────────────────────────
  { FootswitchMode::RamperMomentary,    false, false, false, true,  false, false, "Ramp",          ModulationType::RAMPER,       SHAPE_EXP,    0,  0, false },
  { FootswitchMode::RamperLatching,     true,  false, false, true,  false, false, "Ramp Latch",    ModulationType::RAMPER,       SHAPE_EXP,    0,  0, false },
  { FootswitchMode::RamperInvMomentary, false, false, false, true,  true,  false, "Ramp Inv",      ModulationType::RAMPER,       SHAPE_EXP,    0,  0, false },
  { FootswitchMode::RamperInvLatching,  true,  false, false, true,  true,  false, "Ramp Inv L",    ModulationType::RAMPER,       SHAPE_EXP,    0,  0, false },

  // ── LFO Sine ─────────────────────────────────────────────────────────────
  { FootswitchMode::LfoSineMomentary,   false, false, false, true,  false, false, "LFO Sine",      ModulationType::LFO,          SHAPE_SINE,   0,  0, false },
  { FootswitchMode::LfoSineLatching,    true,  false, false, true,  false, false, "LFO Sine L",    ModulationType::LFO,          SHAPE_SINE,   0,  0, false },

  // ── LFO Triangle ─────────────────────────────────────────────────────────
  { FootswitchMode::LfoTriMomentary,    false, false, false, true,  false, false, "LFO Tri",       ModulationType::LFO,          SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::LfoTriLatching,     true,  false, false, true,  false, false, "LFO Tri L",     ModulationType::LFO,          SHAPE_LINEAR, 0,  0, false },

  // ── LFO Square ───────────────────────────────────────────────────────────
  { FootswitchMode::LfoSqMomentary,     false, false, false, true,  false, false, "LFO Sq",        ModulationType::LFO,          SHAPE_SQUARE, 0,  0, false },
  { FootswitchMode::LfoSqLatching,      true,  false, false, true,  false, false, "LFO Sq L",      ModulationType::LFO,          SHAPE_SQUARE, 0,  0, false },

  // ── Stepper — always linear ──────────────────────────────────────────────
  { FootswitchMode::StepperMomentary,   false, false, false, true,  false, false, "Step",          ModulationType::STEPPER,      SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::StepperLatching,    true,  false, false, true,  false, false, "Step Latch",    ModulationType::STEPPER,      SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::StepperInvMomentary,false, false, false, true,  true,  false, "Step Inv",      ModulationType::STEPPER,      SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::StepperInvLatching, true,  false, false, true,  true,  false, "Step Inv L",    ModulationType::STEPPER,      SHAPE_LINEAR, 0,  0, false },

  // ── Random Stepper ───────────────────────────────────────────────────────
  { FootswitchMode::RandomMomentary,    false, false, false, true,  false, false, "Random",        ModulationType::RANDOM,       SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::RandomLatching,     true,  false, false, true,  false, false, "Random L",      ModulationType::RANDOM,       SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::RandomInvMomentary, false, false, false, true,  true,  false, "Random Inv",    ModulationType::RANDOM,       SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::RandomInvLatching,  true,  false, false, true,  true,  false, "Random Inv L",  ModulationType::RANDOM,       SHAPE_LINEAR, 0,  0, false },

  // ── Scene / Snapshot ─────────────────────────────────────────────────────
  // Each scene mode is followed by its scroll variant. Scroll modes store the
  // user-selected max in midiNumber; each press advances scrollPos 0..max and
  // wraps. sceneCC/sceneMaxVal shared with the non-scroll variant.
  { FootswitchMode::HelixSnapshot,         false, false, false, false, false, true,  "Helix Snap",    ModulationType::NOMODULATION, SHAPE_LINEAR, 69, 7, false, false },
  { FootswitchMode::HelixSnapshotScroll,   false, false, false, false, false, true,  "Helix Scrl",    ModulationType::NOMODULATION, SHAPE_LINEAR, 69, 7, false, true  },
  { FootswitchMode::QuadCortexScene,       false, false, false, false, false, true,  "QC Scene",      ModulationType::NOMODULATION, SHAPE_LINEAR, 43, 7, false, false },
  { FootswitchMode::QuadCortexSceneScroll, false, false, false, false, false, true,  "QC Scrl",       ModulationType::NOMODULATION, SHAPE_LINEAR, 43, 7, false, true  },
  { FootswitchMode::FractalScene,          false, false, false, false, false, true,  "Fractal Scene", ModulationType::NOMODULATION, SHAPE_LINEAR, 34, 7, false, false },
  { FootswitchMode::FractalSceneScroll,    false, false, false, false, false, true,  "Fract Scrl",    ModulationType::NOMODULATION, SHAPE_LINEAR, 34, 7, false, true  },
  // Kemper: encoder picks CC number (CC50–CC54), always sends value 1.
  // Scroll variant cycles through CC50..CC(50+midiNumber) on each press.
  { FootswitchMode::KemperSlot,            false, false, false, false, false, true,  "Kemper Slot",   ModulationType::NOMODULATION, SHAPE_LINEAR, 50, 4, true,  false },
  { FootswitchMode::KemperSlotScroll,      false, false, false, false, false, true,  "Kemper Scrl",   ModulationType::NOMODULATION, SHAPE_LINEAR, 50, 4, true,  true  },

  // ── Tap Tempo ─────────────────────────────────────────────────────────────
  // No MIDI output; press updates the internal clock BPM.
  { FootswitchMode::TapTempo,              false, false, false, false, false, false, "Tap Tempo",     ModulationType::NOMODULATION, SHAPE_LINEAR, 0,  0, false, false, false },

  // ── System / Transport ────────────────────────────────────────────────────
  // Encoder selects a command from systemCommands[]. No MIDI channel used.
  { FootswitchMode::System,                false, false, false, false, false, false, "System",        ModulationType::NOMODULATION, SHAPE_LINEAR, 0,  0, false, false, true  },
};

inline constexpr uint8_t NUM_MODES = sizeof(modes) / sizeof(modes[0]);

// ── Mode categories (for two-level encoder selection) ────────────────────────

struct ModeCategory {
  const char *name;
  uint8_t firstIdx;  // first index into modes[]
  uint8_t count;     // number of modes in this category
};

inline constexpr ModeCategory modeCategories[] = {
  { "Basic",   0,  4 },
  { "Ramper",  4,  4 },
  { "LFO",    8,  6 },
  { "Stepper", 14, 4 },
  { "Random",  18, 4 },
  { "Scenes",  22, 8 },
  { "Utility", 30, 2 },
};
inline constexpr uint8_t NUM_CATEGORIES = sizeof(modeCategories) / sizeof(modeCategories[0]);

inline uint8_t categoryForModeIndex(uint8_t modeIdx) {
  for(uint8_t i = 0; i < NUM_CATEGORIES; i++) {
    if(modeIdx >= modeCategories[i].firstIdx &&
       modeIdx < modeCategories[i].firstIdx + modeCategories[i].count)
      return i;
  }
  return 0;
}

// ── Helpers ──────────────────────────────────────────────────────────────────

inline ModulationType getModulationType(FootswitchMode mode) {
  for(const auto &m : modes) {
    if(m.mode == mode) return m.modType;
  }
  return ModulationType::NOMODULATION;
}

// ── FSButton ─────────────────────────────────────────────────────────────────

struct FSButton {
  uint8_t pin;
  uint8_t ledPin;
  const char *name;

  bool ledState    = false;  // logical LED state (driven externally by main loop)
  bool lastState   = HIGH;
  unsigned long lastDebounce = 0;
  unsigned long rampUpMs     = DEFAULT_RAMP_SPEED;
  unsigned long rampDownMs   = DEFAULT_RAMP_SPEED;
  bool isPressed   = false;
  bool isLatching  = false;
  bool isActivated = false;
  bool isPC        = true;
  bool isNote      = false;
  bool isScene     = false;
  bool isSceneScroll = false;
  bool isSystem    = false;
  uint8_t midiNumber = 0;
  uint8_t modeIndex  = 0;
  uint8_t scrollPos       = 0;  // next scene value to send in scroll mode
  uint8_t scrollLastSent  = 0;  // most recent value sent (for display)
  bool isModSwitch   = false;
  uint8_t fsChannel  = 0xFF;

  FootswitchMode mode = FootswitchMode::MomentaryPC;
  ModeInfo modMode = { FootswitchMode::MomentaryPC, false, true, false, false, false, false,
                       "PC", ModulationType::NOMODULATION, SHAPE_LINEAR, 0, 0, false, false, false };

  FSButton(uint8_t p, uint8_t lp, const char *n, uint8_t mN)
      : pin(p), ledPin(lp), name(n), midiNumber(mN) {}

  void init() {
    pinMode(pin, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
  }

  // Updates logical state only — GPIO is driven by updatePresetLEDs() in main loop.
  void _setLED(bool state) {
    ledState = state;
  }

  uint8_t effectiveChannel(uint8_t globalChannel) const {
    return (fsChannel == 0xFF) ? globalChannel : fsChannel;
  }

  // ── Inner MIDI action — shared by handleFootswitch and simulatePress ───────
  void _applyPressState(bool pressed, uint8_t ch, MidiCCModulator &modulator,
                        void (*displayCallback)(FSButton &)) {
    // Tap Tempo: update the clock BPM on press. No MIDI output.
    // Display is handled in the main loop (needs midiClock.bpm after update).
    if(mode == FootswitchMode::TapTempo) {
      if(pressed) midiClock.receiveTap();
      _setLED(pressed);
      return;
    }

    // System / Transport: dispatch the selected command. No MIDI channel.
    if(isSystem) {
      _setLED(pressed);
      if(pressed) sendSystemCommand(midiNumber);
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(isModSwitch) {
      modulator.modType        = getModulationType(mode);
      modulator.latching       = isLatching;
      // Sync shape + resting position from this button's mode. The modulator
      // is shared across all 6 footswitches, so whichever FS is being pressed
      // must own the shape — otherwise LFO Tri inherits SINE from a previously
      // configured LFO Sine on another FS.
      modulator.rampShape      = modMode.shape;
      if(modMode.isInverted)   modulator.restingHigh = true;
      else                     modulator.restingHigh = false;
      // Resolve clock-sync values to ms at current BPM; plain ms passes through.
      modulator.rampUpTimeMs   = (rampUpMs  & CLOCK_SYNC_FLAG)
                                   ? midiClock.syncToMs(rampUpMs)
                                   : (unsigned long)rampUpMs;
      modulator.rampDownTimeMs = (rampDownMs & CLOCK_SYNC_FLAG)
                                   ? midiClock.syncToMs(rampDownMs)
                                   : (unsigned long)rampDownMs;
      modulator.midiChannel    = ch;
      modulator.midiCCNumber   = midiNumber;
      pressed ? modulator.press() : modulator.release();
      _setLED(modulator.isActivated);
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(isPC) {
      _setLED(pressed);
      if(pressed) sendMIDI(ch, true, midiNumber);
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(isNote) {
      isActivated = pressed;
      _setLED(pressed);
      sendNote(ch, midiNumber, pressed);
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(isScene) {
      _setLED(pressed);
      if(pressed) {
        if(isSceneScroll) {
          // Clamp in case midiNumber (max) shrunk below the current scrollPos.
          if(scrollPos > midiNumber) scrollPos = 0;
          if(modMode.scenePickCC) {
            // Kemper scroll: step through CC50..CC(50+max), value always 1.
            sendMIDI(ch, false, modMode.sceneCC + scrollPos, 1);
          } else {
            // Helix/QC/Fractal scroll: fixed CC, step the value.
            sendMIDI(ch, false, modMode.sceneCC, scrollPos);
          }
          scrollLastSent = scrollPos;
          scrollPos = (scrollPos >= midiNumber) ? 0 : scrollPos + 1;
        } else if(modMode.scenePickCC) {
          sendMIDI(ch, false, modMode.sceneCC + midiNumber, 1);
        } else {
          sendMIDI(ch, false, modMode.sceneCC, midiNumber);
        }
      }
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(isLatching) {
      if(!pressed) return;
      isActivated = !isActivated;
      _setLED(isActivated);
      sendMIDI(ch, false, midiNumber, isActivated ? 127 : 0);
      if(displayCallback) displayCallback(*this);
      return;
    }
    // CC momentary
    isActivated = pressed;
    _setLED(pressed);
    sendMIDI(ch, false, midiNumber, pressed ? 127 : 0);
    if(displayCallback) displayCallback(*this);
  }

  void handleFootswitch(uint8_t midiChannel, MidiCCModulator &modulator,
                        void (*displayCallback)(FSButton &) = nullptr) {
    const uint8_t ch = effectiveChannel(midiChannel);
    if((millis() - lastDebounce) < DEBOUNCE_DELAY) return;
    bool reading = digitalRead(pin);
    bool pressed = (reading == LOW);
    if(reading == lastState) return;
    lastDebounce = millis();
    lastState    = reading;
    isPressed    = pressed;
    _applyPressState(pressed, ch, modulator, displayCallback);
  }

  // Virtual press from web UI — no debounce, no display callback.
  void simulatePress(bool pressed, uint8_t midiChannel, MidiCCModulator &modulator) {
    isPressed = pressed;
    _applyPressState(pressed, effectiveChannel(midiChannel), modulator, nullptr);
  }

  const char *toggleFootswitchMode(MidiCCModulator &modulator) {
    modeIndex = (modeIndex + 1) % NUM_MODES;

    const ModeInfo &m = modes[modeIndex];
    mode        = m.mode;
    isModSwitch = m.isModSwitch;
    isLatching  = m.isLatching;
    isPC        = m.isPC;
    isNote      = m.isNote;
    isScene     = m.isScene;
    isSceneScroll = m.isSceneScroll;
    isSystem    = m.isSystem;
    scrollPos      = 0;
    scrollLastSent = 0;
    modMode     = m;

    if(m.isInverted) {
      modulator.setRestingHigh();
    } else {
      modulator.setRestingLow();
    }
    modulator.rampShape = m.shape;
    modulator.reset();

    isActivated = false;
    _setLED(false);
    return m.name;
  }
};

// ── Free mode-application helpers (defined after FSButton) ───────────────────

// Apply mode flags to a button without touching the modulator.
// Used for preset loading and NVS restore.
inline void applyModeFlags(FSButton &btn, uint8_t idx) {
  if(idx >= NUM_MODES) return;
  const ModeInfo &m = modes[idx];
  btn.modeIndex      = idx;
  btn.mode           = m.mode;
  btn.isModSwitch    = m.isModSwitch;
  btn.isLatching     = m.isLatching;
  btn.isPC           = m.isPC;
  btn.isNote         = m.isNote;
  btn.isScene        = m.isScene;
  btn.isSceneScroll  = m.isSceneScroll;
  btn.isSystem       = m.isSystem;
  btn.scrollPos      = 0;
  btn.scrollLastSent = 0;
  btn.modMode        = m;
  btn.isActivated    = false;
  btn.ledState       = false;
}

// Apply mode and optionally reset the modulator (used by web server and encoder button).
inline void applyModeIndex(FSButton &btn, uint8_t idx, MidiCCModulator *mod = nullptr) {
  applyModeFlags(btn, idx);
  if(mod) {
    const ModeInfo &m = modes[idx];
    if(m.isInverted) mod->setRestingHigh();
    else              mod->setRestingLow();
    mod->rampShape = m.shape;
    mod->reset();
  }
}
