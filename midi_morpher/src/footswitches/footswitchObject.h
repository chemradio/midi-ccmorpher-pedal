#pragma once
#include "../config.h"
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

  // Scene / Snapshot
  HelixSnapshot,
  QuadCortexScene,
  FractalScene,
  KemperSlot,
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
};

// ── Mode table ───────────────────────────────────────────────────────────────
// Single source of truth for all 26 modes.
// Columns: mode, isLatching, isPC, isNote, isModSwitch, isInverted, isScene,
//          name, modType, shape, sceneCC, sceneMaxVal, scenePickCC

static constexpr ModeInfo modes[] = {
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

  // ── Stepper — exponential curve, same as ramper per spec ─────────────────
  { FootswitchMode::StepperMomentary,   false, false, false, true,  false, false, "Step",          ModulationType::STEPPER,      SHAPE_EXP,    0,  0, false },
  { FootswitchMode::StepperLatching,    true,  false, false, true,  false, false, "Step Latch",    ModulationType::STEPPER,      SHAPE_EXP,    0,  0, false },
  { FootswitchMode::StepperInvMomentary,false, false, false, true,  true,  false, "Step Inv",      ModulationType::STEPPER,      SHAPE_EXP,    0,  0, false },
  { FootswitchMode::StepperInvLatching, true,  false, false, true,  true,  false, "Step Inv L",    ModulationType::STEPPER,      SHAPE_EXP,    0,  0, false },

  // ── Random Stepper ───────────────────────────────────────────────────────
  { FootswitchMode::RandomMomentary,    false, false, false, true,  false, false, "Random",        ModulationType::RANDOM,       SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::RandomLatching,     true,  false, false, true,  false, false, "Random L",      ModulationType::RANDOM,       SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::RandomInvMomentary, false, false, false, true,  true,  false, "Random Inv",    ModulationType::RANDOM,       SHAPE_LINEAR, 0,  0, false },
  { FootswitchMode::RandomInvLatching,  true,  false, false, true,  true,  false, "Random Inv L",  ModulationType::RANDOM,       SHAPE_LINEAR, 0,  0, false },

  // ── Scene / Snapshot ─────────────────────────────────────────────────────
  // sceneCC = base CC, sceneMaxVal = encoder ceiling, scenePickCC = encoder selects CC# (Kemper)
  { FootswitchMode::HelixSnapshot,      false, false, false, false, false, true,  "Helix Snap",    ModulationType::NOMODULATION, SHAPE_LINEAR, 69, 7, false },
  { FootswitchMode::QuadCortexScene,    false, false, false, false, false, true,  "QC Scene",      ModulationType::NOMODULATION, SHAPE_LINEAR, 43, 7, false },
  { FootswitchMode::FractalScene,       false, false, false, false, false, true,  "Fractal Scene", ModulationType::NOMODULATION, SHAPE_LINEAR, 34, 7, false },
  // Kemper: encoder picks CC number (CC50–CC54), always sends value 1
  { FootswitchMode::KemperSlot,         false, false, false, false, false, true,  "Kemper Slot",   ModulationType::NOMODULATION, SHAPE_LINEAR, 50, 4, true  },
};

static constexpr uint8_t NUM_MODES = sizeof(modes) / sizeof(modes[0]);

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

  bool ledState    = false;
  bool lastState   = HIGH;
  unsigned long lastDebounce = 0;
  unsigned long rampUpMs     = DEFAULT_RAMP_SPEED; // per-footswitch UP ramp time
  unsigned long rampDownMs   = DEFAULT_RAMP_SPEED; // per-footswitch DOWN ramp time
  bool isPressed   = false;
  bool isLatching  = false;
  bool isActivated = false;
  bool isPC        = true;
  bool isNote      = false;
  bool isScene     = false;
  uint8_t midiNumber = 0;
  uint8_t modeIndex  = 0;
  bool isModSwitch   = false;
  uint8_t fsChannel  = 0xFF; // 0xFF = follow global MIDI channel; 0–15 = per-FS override

  FootswitchMode mode = FootswitchMode::MomentaryPC;
  ModeInfo modMode = { FootswitchMode::MomentaryPC, false, true, false, false, false, false,
                       "PC", ModulationType::NOMODULATION, SHAPE_LINEAR, 0, 0, false };

  FSButton(uint8_t p, uint8_t lp, const char *n, uint8_t mN)
      : pin(p), ledPin(lp), name(n), midiNumber(mN) {}

  void init() {
    pinMode(pin, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
  }

  void _setLED(bool state) {
    ledState = state;
    digitalWrite(ledPin, state ? HIGH : LOW);
  }

  // Returns the channel this footswitch should use — per-FS override or global fallback.
  uint8_t effectiveChannel(uint8_t globalChannel) const {
    return (fsChannel == 0xFF) ? globalChannel : fsChannel;
  }

  void handleFootswitch(uint8_t midiChannel, MidiCCModulator &modulator, void (*displayCallback)(FSButton &) = nullptr) {
    const uint8_t ch = effectiveChannel(midiChannel);
    if((millis() - lastDebounce) < DEBOUNCE_DELAY) return;

    bool reading = digitalRead(pin);
    bool pressed = (reading == LOW);
    if(reading == lastState) return;

    lastDebounce = millis();
    lastState    = reading;
    isPressed    = pressed;

    // ── Modulation ───────────────────────────────────────────────────────
    if(isModSwitch) {
      modulator.modType        = getModulationType(mode);
      modulator.latching       = isLatching;
      modulator.rampUpTimeMs   = rampUpMs;
      modulator.rampDownTimeMs = rampDownMs;
      modulator.midiChannel    = ch;  // use per-FS channel for all CC output
      pressed ? modulator.press() : modulator.release();
      _setLED(modulator.isActivated);
      if(displayCallback) displayCallback(*this);
      return;
    }

    // ── Program Change ───────────────────────────────────────────────────
    if(isPC) {
      _setLED(pressed);
      if(pressed) sendMIDI(ch, true, midiNumber);
      if(displayCallback) displayCallback(*this);
      return;
    }

    // ── Note ─────────────────────────────────────────────────────────────
    if(isNote) {
      isActivated = pressed;
      _setLED(pressed);
      sendNote(ch, midiNumber, pressed);
      if(displayCallback) displayCallback(*this);
      return;
    }

    // ── Scene / Snapshot ─────────────────────────────────────────────────
    if(isScene) {
      _setLED(pressed);
      if(pressed) {
        if(modMode.scenePickCC) {
          sendMIDI(ch, false, modMode.sceneCC + midiNumber, 1);
        } else {
          sendMIDI(ch, false, modMode.sceneCC, midiNumber);
        }
      }
      if(displayCallback) displayCallback(*this);
      return;
    }

    // ── CC latching ───────────────────────────────────────────────────────
    if(isLatching) {
      if(!pressed) return;
      isActivated = !isActivated;
      _setLED(isActivated);
      sendMIDI(ch, false, midiNumber, isActivated ? 127 : 0);
      if(displayCallback) displayCallback(*this);
      return;
    }

    // ── CC momentary ─────────────────────────────────────────────────────
    isActivated = pressed;
    _setLED(pressed);
    sendMIDI(ch, false, midiNumber, pressed ? 127 : 0);
    if(displayCallback) displayCallback(*this);
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
    modMode     = m;

    if(m.isInverted) {
      modulator.setRestingHigh();
    } else {
      modulator.setRestingLow();
    }
    modulator.rampShape = m.shape;
    modulator.reset();

    // Always start with LED off — active state begins at rest.
    isActivated = false;
    _setLED(false);
    return m.name;
  }
};
