#pragma once
#include "../config.h"
#include "../clock/midiClock.h"
#include "../hidKeyboard.h"
#include "../midiCCModulator.h"
#include "../midiOut.h"
#include "../sharedTypes.h"
#include "../multimode/multiScene.h"

static constexpr uint8_t PB_SENTINEL = 0xFF;

// ── Extra press action ────────────────────────────────────────────────────────
// Per-footswitch config for LONG(0), DOUBLE(1), and RELEASE(2) press events.
struct FSAction {
    bool     enabled    = false;
    uint8_t  modeIndex  = 0;
    uint8_t  midiNumber = 0;
    uint8_t  fsChannel  = 0xFF;
    uint8_t  ccLow      = 0;
    uint8_t  ccHigh     = 127;
    uint8_t  velocity   = 100;
    uint32_t rampUpMs   = DEFAULT_RAMP_SPEED;
    uint32_t rampDownMs = DEFAULT_RAMP_SPEED;
};
static constexpr uint8_t        FS_NUM_EXTRA  = 3;
static constexpr unsigned long  FS_LONG_MS    = 500;
static constexpr unsigned long  FS_DBL_WIN_MS = 300;

enum class PressPhase : uint8_t { IDLE, WAIT, PRESS_ACTIVE, LONG_ACTIVE, PENDING_DBL, DBL_ACTIVE };

// ── Footswitch mode enum ─────────────────────────────────────────────────────

enum class FootswitchMode {
  // No Action — does nothing; default for extra action slots (index 0)
  NoAction,

  // Basic
  MomentaryPC,
  MomentaryCC,
  LatchingCC,
  SingleCC,
  MomentaryNote,

  // Ramper — Exp curve
  RamperMomentary,
  RamperLatching,
  RamperInvMomentary,
  RamperInvLatching,

  // Ramper — Linear curve
  RamperLinMomentary,
  RamperLinLatching,
  RamperLinInvMomentary,
  RamperLinInvLatching,

  // Ramper — Sine curve
  RamperSineMomentary,
  RamperSineLatching,
  RamperSineInvMomentary,
  RamperSineInvLatching,

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
  HelixSnapshotScroll,
  QuadCortexScene,
  QuadCortexSceneScroll,
  FractalScene,
  FractalSceneScroll,
  KemperSlot,
  KemperSlotScroll,

  // Utility
  TapTempo,
  System,

  // USB HID keyboard keystroke
  Keyboard,

  // Multi — fires multiple sub-commands at once; midiNumber = scene slot index
  Multi,

  // Preset Navigation
  PresetUp,
  PresetDown,
  PresetNum,   // jump to a specific preset; midiNumber = preset index (0–5)
};

// ── ModeInfo ─────────────────────────────────────────────────────────────────

struct ModeInfo {
  FootswitchMode mode;
  bool isLatching;
  bool isPC;
  bool isNote;
  bool isModSwitch;
  bool isInverted;
  bool isScene;
  const char *name;
  ModulationType modType;
  RampShape shape;
  uint8_t sceneCC;
  uint8_t sceneMaxVal;
  bool scenePickCC;
  bool isSceneScroll;
  bool isSystem;
  bool isKeyboard;
};

// ── System / Transport command table ────────────────────────────────────────

enum SysCmdKind : uint8_t { SYS_MMC, SYS_SPP };

struct SysCmdInfo {
  const char *name;
  SysCmdKind  kind;
  uint8_t     data;
  uint8_t     rt;
};

inline constexpr SysCmdInfo systemCommands[] = {
  { "Play",      SYS_MMC, 0x02, 0xFA },
  { "Stop",      SYS_MMC, 0x01, 0xFC },
  { "Continue",  SYS_MMC, 0x03, 0xFB },
  { "Record",    SYS_MMC, 0x06, 0x00 },
  { "Pause",     SYS_MMC, 0x09, 0x00 },
  { "Rewind",    SYS_MMC, 0x05, 0x00 },
  { "FFwd",      SYS_MMC, 0x04, 0x00 },
  { "GotoStart", SYS_SPP, 0x00, 0x00 },
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
// Columns: mode, isLatching, isPC, isNote, isModSwitch, isInverted, isScene,
//          name, modType, shape, sceneCC, sceneMaxVal, scenePickCC,
//          isSceneScroll, isSystem, isKeyboard

inline constexpr ModeInfo modes[] = {
  // ── No Action (index 0) ───────────────────────────────────────────────────
  { FootswitchMode::NoAction,              false,false,false,false,false,false,"No Action",     ModulationType::NOMODULATION,SHAPE_LINEAR,0, 0,false,false,false,false },

  // ── Basic (index 1–5) ─────────────────────────────────────────────────────
  { FootswitchMode::MomentaryPC,        false,true, false,false,false,false,"PC",            ModulationType::NOMODULATION,SHAPE_LINEAR,0, 0,false,false,false,false },
  { FootswitchMode::MomentaryCC,        false,false,false,false,false,false,"CC",            ModulationType::NOMODULATION,SHAPE_LINEAR,0, 0,false,false,false,false },
  { FootswitchMode::LatchingCC,         true, false,false,false,false,false,"CC Latch",      ModulationType::NOMODULATION,SHAPE_LINEAR,0, 0,false,false,false,false },
  { FootswitchMode::SingleCC,           false,false,false,false,false,false,"CC Single",     ModulationType::NOMODULATION,SHAPE_LINEAR,0, 0,false,false,false,false },
  { FootswitchMode::MomentaryNote,      false,false,true, false,false,false,"Note",          ModulationType::NOMODULATION,SHAPE_LINEAR,0, 0,false,false,false,false },

  // ── Ramper Exp (index 6–9) ───────────────────────────────────────────────
  { FootswitchMode::RamperMomentary,       false,false,false,true, false,false,"Ramp",          ModulationType::RAMPER, SHAPE_EXP,   0,0,false,false,false,false },
  { FootswitchMode::RamperLatching,        true, false,false,true, false,false,"Ramp Latch",    ModulationType::RAMPER, SHAPE_EXP,   0,0,false,false,false,false },
  { FootswitchMode::RamperInvMomentary,    false,false,false,true, true, false,"Ramp Inv",      ModulationType::RAMPER, SHAPE_EXP,   0,0,false,false,false,false },
  { FootswitchMode::RamperInvLatching,     true, false,false,true, true, false,"Ramp Inv L",    ModulationType::RAMPER, SHAPE_EXP,   0,0,false,false,false,false },

  // ── Ramper Linear (index 10–13) ──────────────────────────────────────────
  { FootswitchMode::RamperLinMomentary,    false,false,false,true, false,false,"Ramp Lin",      ModulationType::RAMPER, SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::RamperLinLatching,     true, false,false,true, false,false,"Ramp Lin L",    ModulationType::RAMPER, SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::RamperLinInvMomentary, false,false,false,true, true, false,"Ramp Lin Inv",  ModulationType::RAMPER, SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::RamperLinInvLatching,  true, false,false,true, true, false,"Ramp Lin Inv L",ModulationType::RAMPER, SHAPE_LINEAR,0,0,false,false,false,false },

  // ── Ramper Sine (index 14–17) ────────────────────────────────────────────
  { FootswitchMode::RamperSineMomentary,    false,false,false,true, false,false,"Ramp Sine",     ModulationType::RAMPER, SHAPE_SINE,  0,0,false,false,false,false },
  { FootswitchMode::RamperSineLatching,     true, false,false,true, false,false,"Ramp Sine L",   ModulationType::RAMPER, SHAPE_SINE,  0,0,false,false,false,false },
  { FootswitchMode::RamperSineInvMomentary, false,false,false,true, true, false,"Ramp Sine Inv", ModulationType::RAMPER, SHAPE_SINE,  0,0,false,false,false,false },
  { FootswitchMode::RamperSineInvLatching,  true, false,false,true, true, false,"Ramp Sine Inv L",ModulationType::RAMPER,SHAPE_SINE,  0,0,false,false,false,false },

  // ── LFO Sine (index 18–19) ───────────────────────────────────────────────
  { FootswitchMode::LfoSineMomentary,   false,false,false,true, false,false,"LFO Sine",      ModulationType::LFO,    SHAPE_SINE,  0,0,false,false,false,false },
  { FootswitchMode::LfoSineLatching,    true, false,false,true, false,false,"LFO Sine L",    ModulationType::LFO,    SHAPE_SINE,  0,0,false,false,false,false },

  // ── LFO Triangle (index 20–21) ───────────────────────────────────────────
  { FootswitchMode::LfoTriMomentary,    false,false,false,true, false,false,"LFO Tri",       ModulationType::LFO,    SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::LfoTriLatching,     true, false,false,true, false,false,"LFO Tri L",     ModulationType::LFO,    SHAPE_LINEAR,0,0,false,false,false,false },

  // ── LFO Square (index 22–23) ─────────────────────────────────────────────
  { FootswitchMode::LfoSqMomentary,     false,false,false,true, false,false,"LFO Sq",        ModulationType::LFO,    SHAPE_SQUARE,0,0,false,false,false,false },
  { FootswitchMode::LfoSqLatching,      true, false,false,true, false,false,"LFO Sq L",      ModulationType::LFO,    SHAPE_SQUARE,0,0,false,false,false,false },

  // ── Stepper (index 24–27) ────────────────────────────────────────────────
  { FootswitchMode::StepperMomentary,   false,false,false,true, false,false,"Step",          ModulationType::STEPPER,SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::StepperLatching,    true, false,false,true, false,false,"Step Latch",    ModulationType::STEPPER,SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::StepperInvMomentary,false,false,false,true, true, false,"Step Inv",      ModulationType::STEPPER,SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::StepperInvLatching, true, false,false,true, true, false,"Step Inv L",    ModulationType::STEPPER,SHAPE_LINEAR,0,0,false,false,false,false },

  // ── Random Stepper (index 28–31) ─────────────────────────────────────────
  { FootswitchMode::RandomMomentary,    false,false,false,true, false,false,"Random",        ModulationType::RANDOM, SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::RandomLatching,     true, false,false,true, false,false,"Random L",      ModulationType::RANDOM, SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::RandomInvMomentary, false,false,false,true, true, false,"Random Inv",    ModulationType::RANDOM, SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::RandomInvLatching,  true, false,false,true, true, false,"Random Inv L",  ModulationType::RANDOM, SHAPE_LINEAR,0,0,false,false,false,false },

  // ── Scene / Snapshot (index 32–39) ───────────────────────────────────────
  { FootswitchMode::HelixSnapshot,         false,false,false,false,false,true, "Helix Snap",    ModulationType::NOMODULATION,SHAPE_LINEAR,69,7,false,false,false,false },
  { FootswitchMode::HelixSnapshotScroll,   false,false,false,false,false,true, "Helix Scrl",    ModulationType::NOMODULATION,SHAPE_LINEAR,69,7,false,true, false,false },
  { FootswitchMode::QuadCortexScene,       false,false,false,false,false,true, "QC Scene",      ModulationType::NOMODULATION,SHAPE_LINEAR,43,7,false,false,false,false },
  { FootswitchMode::QuadCortexSceneScroll, false,false,false,false,false,true, "QC Scrl",       ModulationType::NOMODULATION,SHAPE_LINEAR,43,7,false,true, false,false },
  { FootswitchMode::FractalScene,          false,false,false,false,false,true, "Fractal Scene", ModulationType::NOMODULATION,SHAPE_LINEAR,34,7,false,false,false,false },
  { FootswitchMode::FractalSceneScroll,    false,false,false,false,false,true, "Fract Scrl",    ModulationType::NOMODULATION,SHAPE_LINEAR,34,7,false,true, false,false },
  { FootswitchMode::KemperSlot,            false,false,false,false,false,true, "Kemper Slot",   ModulationType::NOMODULATION,SHAPE_LINEAR,50,4,true, false,false,false },
  { FootswitchMode::KemperSlotScroll,      false,false,false,false,false,true, "Kemper Scrl",   ModulationType::NOMODULATION,SHAPE_LINEAR,50,4,true, true, false,false },

  // ── Utility (index 40–41) ────────────────────────────────────────────────
  { FootswitchMode::TapTempo,              false,false,false,false,false,false,"Tap Tempo",     ModulationType::NOMODULATION,SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::System,                false,false,false,false,false,false,"System",        ModulationType::NOMODULATION,SHAPE_LINEAR,0,0,false,false,true, false },

  // ── Keyboard (index 42) ──────────────────────────────────────────────────
  // midiNumber = index into hidKeys[]. fsChannel = modifier bitmask (KEY_MOD_*).
  { FootswitchMode::Keyboard,              false,false,false,false,false,false,"Keyboard",      ModulationType::NOMODULATION,SHAPE_LINEAR,0,0,false,false,false,true  },

  // ── Multi (index 43) ─────────────────────────────────────────────────────
  // midiNumber = slot index into multiScenes[]. No mode flags set.
  { FootswitchMode::Multi,                 false,false,false,false,false,false,"Multi",         ModulationType::NOMODULATION,SHAPE_LINEAR,0,0,false,false,false,false },

  // ── Preset Navigation (index 44–46) ──────────────────────────────────────
  { FootswitchMode::PresetUp,              false,false,false,false,false,false,"Preset Up",     ModulationType::NOMODULATION,SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::PresetDown,            false,false,false,false,false,false,"Preset Down",   ModulationType::NOMODULATION,SHAPE_LINEAR,0,0,false,false,false,false },
  { FootswitchMode::PresetNum,             false,false,false,false,false,false,"Preset #",      ModulationType::NOMODULATION,SHAPE_LINEAR,0,0,false,false,false,false },
};

inline constexpr uint8_t NUM_MODES = sizeof(modes) / sizeof(modes[0]); // 39

// ── Mode categories (three-level encoder selection) ──────────────────────────
//
// autoSelect=true  → single mode, confirmed immediately on category press
// subGroupCount=0  → two nav levels: cat → variant
// subGroupCount>0  → three nav levels: cat → sub-group → variant
//                    modeIdx = firstIdx + sgIdx*subGroupSize + variantIdx
//
// Ramp/Stepper/Random: subGroups = Normal / Inverted  (sgCount=2, sgSize=2)
// LFO:                 subGroups = Sine / Tri / Sq    (sgCount=3, sgSize=2)
// CC / Scenes:         no sub-groups                  (sgCount=0)

struct ModeCategory {
  const char *name;
  bool autoSelect;
  uint8_t firstIdx;
  uint8_t count;
  uint8_t subGroupCount;
  uint8_t subGroupSize;
  const char * const *subGroupNames;
  const char * const *subGroupNotes;
  const char * const *variantNames;
};

// String tables for sub-groups and variants
inline constexpr const char *sgNormalInv[]  = { "Normal",   "Inverted"  };
inline constexpr const char *sgNormNotes[]  = { "0 > 127",  "127 > 0"   };
inline constexpr const char *sgRampWave[]   = { "Exp",     "Exp Inv",  "Linear", "Lin Inv", "Sine",   "Sine Inv" };
inline constexpr const char *sgRampNotes[]  = { "0>127",   "127>0",    "0>127",  "127>0",   "smooth", "smooth"   };
inline constexpr const char *sgLfoWave[]    = { "Sine",     "Triangle", "Square"   };
inline constexpr const char *sgLfoNotes[]   = { "smooth",   "linear",   "on/off"   };
inline constexpr const char *varMomLatch[]     = { "Momentary","Latching"  };
inline constexpr const char *varCC[]           = { "Momentary","Latching","Single" };
inline constexpr const char *varPresetNav[]    = { "Up",       "Down",     "Preset #" };
inline constexpr const char *varHelix[]     = { "Snapshot", "Scroll"    };
inline constexpr const char *varQC[]        = { "Scene",    "Scroll"    };
inline constexpr const char *varFractal[]   = { "Scene",    "Scroll"    };
inline constexpr const char *varKemper[]    = { "Slot",     "Scroll"    };

inline constexpr ModeCategory modeCategories[] = {
  //  name,         auto,  first,count, sgCnt,sgSz, sgNames,    sgNotes,    varNames
  { "No Action",    true,  0,  1,  0, 0, nullptr,     nullptr,     nullptr      },
  { "PC",           true,  1,  1,  0, 0, nullptr,     nullptr,     nullptr      },
  { "CC",           false, 2,  3,  0, 0, nullptr,     nullptr,     varCC        },
  { "Note",         true,  5,  1,  0, 0, nullptr,     nullptr,     nullptr      },
  { "Ramp",         false, 6,  12, 6, 2, sgRampWave,  sgRampNotes, varMomLatch  },
  { "LFO",          false, 18, 6,  3, 2, sgLfoWave,   sgLfoNotes,  varMomLatch  },
  { "Stepper",      false, 24, 4,  2, 2, sgNormalInv, sgNormNotes, varMomLatch  },
  { "Random",       false, 28, 4,  2, 2, sgNormalInv, sgNormNotes, varMomLatch  },
  { "Helix",        false, 32, 2,  0, 0, nullptr,     nullptr,     varHelix     },
  { "Quad Cortex",  false, 34, 2,  0, 0, nullptr,     nullptr,     varQC        },
  { "Fractal",      false, 36, 2,  0, 0, nullptr,     nullptr,     varFractal   },
  { "Kemper",       false, 38, 2,  0, 0, nullptr,     nullptr,     varKemper    },
  { "Tap Tempo",    true,  40, 1,  0, 0, nullptr,     nullptr,     nullptr      },
  { "System",       true,  41, 1,  0, 0, nullptr,     nullptr,     nullptr      },
  { "Keyboard",     true,  42, 1,  0, 0, nullptr,     nullptr,     nullptr      },
  { "Multi",        false, 43, 1,  0, 0, nullptr,     nullptr,     nullptr      },
  { "Preset Nav",   false, 44, 3,  0, 0, nullptr,     nullptr,     varPresetNav },
};
inline constexpr uint8_t NUM_CATEGORIES = sizeof(modeCategories) / sizeof(modeCategories[0]); // 17

// Set to +1 (PresetUp) or -1 (PresetDown) on press; cleared by the main loop.
inline int8_t presetNavRequest = 0;
// Set to 0–5 for PresetNum direct-jump; -1 = no request.
inline int8_t presetNavDirect  = -1;

inline uint8_t categoryForModeIndex(uint8_t modeIdx) {
  for(uint8_t i = 0; i < NUM_CATEGORIES; i++) {
    if(modeIdx >= modeCategories[i].firstIdx &&
       modeIdx <  modeCategories[i].firstIdx + modeCategories[i].count)
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

  bool ledState    = false;
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
  bool isKeyboard  = false;
  uint8_t midiNumber = 0;
  uint8_t modeIndex  = 0;
  uint8_t scrollPos       = 0;
  uint8_t scrollLastSent  = 0;
  bool isModSwitch   = false;
  uint8_t fsChannel  = 0xFF;  // 0xFF=global; in Keyboard mode: KEY_MOD_* bitmask
  uint8_t ccLow      = 0;     // value sent on CC release / latch-off
  uint8_t ccHigh     = 127;   // value sent on CC press / latch-on
  uint8_t velocity   = 100;   // Note On velocity

  FootswitchMode mode = FootswitchMode::MomentaryPC;
  ModeInfo modMode = { FootswitchMode::MomentaryPC, false,true,false,false,false,false,
                       "PC",ModulationType::NOMODULATION,SHAPE_LINEAR,0,0,false,false,false,false };

  // Extra press actions: [0]=LONG [1]=DOUBLE [2]=RELEASE
  FSAction extraActions[FS_NUM_EXTRA];

  // Press state machine
  PressPhase    pressPhase  = PressPhase::IDLE;
  unsigned long pressStartMs = 0;
  unsigned long releaseMs    = 0;
  bool          longFired    = false;

  FSButton(uint8_t p, uint8_t lp, const char *n, uint8_t mN)
      : pin(p), ledPin(lp), name(n), midiNumber(mN) {}

  void init() {
    pinMode(pin, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
  }

  void _setLED(bool state) { ledState = state; }

  uint8_t effectiveChannel(uint8_t globalChannel) const {
    return (fsChannel == 0xFF) ? globalChannel : fsChannel;
  }

  void _applyPressState(bool pressed, uint8_t ch, MidiCCModulator &modulator,
                        void (*displayCallback)(FSButton &)) {
    if(mode == FootswitchMode::NoAction) return;
    if(mode == FootswitchMode::TapTempo) {
      if(pressed) midiClock.receiveTap();
      _setLED(pressed);
      return;
    }

    if(mode == FootswitchMode::PresetUp || mode == FootswitchMode::PresetDown) {
      if(pressed) presetNavRequest = (mode == FootswitchMode::PresetUp) ? +1 : -1;
      _setLED(pressed);
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(mode == FootswitchMode::PresetNum) {
      if(pressed) presetNavDirect = (int8_t)(midiNumber < NUM_PRESETS ? midiNumber : NUM_PRESETS - 1);
      _setLED(pressed);
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(isKeyboard) {
      if(pressed) sendKeyDown(midiNumber, fsChannel == 0xFF ? 0 : fsChannel);
      else        sendKeyUp();
      isActivated = pressed;
      _setLED(pressed);
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(isSystem) {
      _setLED(pressed);
      if(pressed) sendSystemCommand(midiNumber);
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(mode == FootswitchMode::Multi) {
      _setLED(pressed);
      uint8_t si = midiNumber;
      if(si < MAX_MULTI_SCENES && multiScenes[si].name[0] != '\0') {
        const MultiScene &sc = multiScenes[si];
        for(uint8_t s = 0; s < sc.count && s < MAX_MULTI_SUBCMDS; s++) {
          const MultiSubCmd &sub = sc.subCmds[s];
          if(sub.modeIndex >= NUM_MODES) continue;
          const ModeInfo &mi = modes[sub.modeIndex];
          uint8_t eff = (sub.channel == 0xFF) ? ch : sub.channel;
          if(mi.isPC)        { if(pressed) sendMIDI(eff, true, sub.midiNumber); }
          else if(mi.isNote) { sendNote(eff, sub.midiNumber, pressed); }
          else if(mi.isScene) {
            if(pressed) {
              if(mi.scenePickCC) sendMIDI(eff, false, mi.sceneCC + sub.midiNumber, 1);
              else               sendMIDI(eff, false, mi.sceneCC, sub.midiNumber);
            }
          }
          else if(mi.isSystem) { if(pressed) sendSystemCommand(sub.midiNumber); }
          else { sendMIDI(eff, false, sub.midiNumber, pressed ? 127 : 0); }
        }
      }
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(isModSwitch) {
      modulator.modType        = getModulationType(mode);
      modulator.latching       = isLatching;
      modulator.rampShape      = modMode.shape;
      if(modMode.isInverted)   modulator.restingHigh = true;
      else                     modulator.restingHigh = false;
      ModDest newDest = (midiNumber == PB_SENTINEL) ? DEST_PITCHBEND : DEST_CC;
      if(modulator.destType != newDest) {
        modulator.destType     = newDest;
        modulator.currentValue = modulator.restVal();
        modulator.lastEmittedCC = 0xFF;
        modulator.lastEmittedPB = 0xFFFF;
      }
      modulator.destType       = newDest;
      modulator.rampUpTimeMs   = (rampUpMs  & CLOCK_SYNC_FLAG)
                                   ? midiClock.syncToMs(rampUpMs)
                                   : (unsigned long)rampUpMs;
      modulator.rampDownTimeMs = (rampDownMs & CLOCK_SYNC_FLAG)
                                   ? midiClock.syncToMs(rampDownMs)
                                   : (unsigned long)rampDownMs;
      modulator.midiChannel    = ch;
      modulator.midiCCNumber   = (newDest == DEST_PITCHBEND) ? 0 : midiNumber;
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
      sendNote(ch, midiNumber, pressed, velocity);
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(isScene) {
      _setLED(pressed);
      if(pressed) {
        if(isSceneScroll) {
          if(scrollPos > midiNumber) scrollPos = 0;
          if(modMode.scenePickCC) {
            sendMIDI(ch, false, modMode.sceneCC + scrollPos, 1);
          } else {
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
    if(mode == FootswitchMode::SingleCC) {
      isActivated = pressed;
      _setLED(pressed);
      if(pressed) sendMIDI(ch, false, midiNumber, ccHigh);
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(isLatching) {
      if(!pressed) return;
      isActivated = !isActivated;
      _setLED(isActivated);
      sendMIDI(ch, false, midiNumber, isActivated ? ccHigh : ccLow);
      if(displayCallback) displayCallback(*this);
      return;
    }
    isActivated = pressed;
    _setLED(pressed);
    sendMIDI(ch, false, midiNumber, pressed ? ccHigh : ccLow);
    if(displayCallback) displayCallback(*this);
  }

  // Fire the RELEASE extra action.
  // Always clears switchPressed first so press() is never blocked by prior state
  // (e.g. primary-mode modulation or an earlier release that left it set).
  void _fireReleaseAction(uint8_t ch, MidiCCModulator &modulator,
                          void (*displayCallback)(FSButton &)) {
    modulator.switchPressed = false;
    _applyExtraAction(2, true, ch, modulator, displayCallback);
    modulator.switchPressed = false;  // clear after so the next release also fires
  }

  void _applyExtraAction(uint8_t t, bool pressed, uint8_t ch,
                         MidiCCModulator &modulator,
                         void (*displayCallback)(FSButton &)) {
    if(t >= FS_NUM_EXTRA || !extraActions[t].enabled) return;
    const FSAction &act = extraActions[t];
    if(act.modeIndex >= NUM_MODES) return;
    const ModeInfo &mi = modes[act.modeIndex];
    uint8_t ach = (act.fsChannel == 0xFF) ? ch : act.fsChannel;

    // Save primary config
    uint8_t  sv_mi=modeIndex; FootswitchMode sv_mo=mode; ModeInfo sv_mm=modMode;
    uint8_t  sv_mn=midiNumber, sv_fc=fsChannel, sv_cl=ccLow, sv_cH=ccHigh, sv_vel=velocity;
    uint32_t sv_ru=rampUpMs, sv_rd=rampDownMs;
    bool sv_lat=isLatching, sv_pc=isPC, sv_nt=isNote, sv_ms=isModSwitch;
    bool sv_sc=isScene, sv_ss=isSceneScroll, sv_sy=isSystem, sv_kb=isKeyboard;

    modeIndex=act.modeIndex; midiNumber=act.midiNumber; fsChannel=act.fsChannel;
    ccLow=act.ccLow; ccHigh=act.ccHigh; velocity=act.velocity;
    rampUpMs=act.rampUpMs; rampDownMs=act.rampDownMs;
    mode=mi.mode; modMode=mi;
    isLatching=mi.isLatching; isPC=mi.isPC; isNote=mi.isNote;
    isModSwitch=mi.isModSwitch; isScene=mi.isScene; isSceneScroll=mi.isSceneScroll;
    isSystem=mi.isSystem; isKeyboard=mi.isKeyboard;

    _applyPressState(pressed, ach, modulator, displayCallback);

    modeIndex=sv_mi; mode=sv_mo; modMode=sv_mm;
    midiNumber=sv_mn; fsChannel=sv_fc; ccLow=sv_cl; ccHigh=sv_cH; velocity=sv_vel;
    rampUpMs=sv_ru; rampDownMs=sv_rd;
    isLatching=sv_lat; isPC=sv_pc; isNote=sv_nt; isModSwitch=sv_ms;
    isScene=sv_sc; isSceneScroll=sv_ss; isSystem=sv_sy; isKeyboard=sv_kb;
  }

  void handleFootswitch(uint8_t midiChannel, MidiCCModulator &modulator,
                        void (*displayCallback)(FSButton &) = nullptr) {
    unsigned long now = millis();
    const uint8_t ch  = effectiveChannel(midiChannel);
    bool hasLong    = extraActions[0].enabled && extraActions[0].modeIndex != 0;
    bool hasDouble  = extraActions[1].enabled && extraActions[1].modeIndex != 0;
    bool hasRelease = extraActions[2].enabled && extraActions[2].modeIndex != 0;

    // Debounced edge detection
    bool edgeDetected = false;
    if((now - lastDebounce) >= DEBOUNCE_DELAY) {
      bool reading = digitalRead(pin);
      if(reading != lastState) {
        lastDebounce = now;
        lastState    = reading;
        isPressed    = (reading == LOW);
        edgeDetected = true;
      }
    }

    if(edgeDetected) {
      if(isPressed) {
        // Press edge
        if(pressPhase == PressPhase::PENDING_DBL) {
          // Second press within double-tap window → fire DOUBLE
          pressPhase = PressPhase::DBL_ACTIVE;
          _applyExtraAction(1, true, ch, modulator, displayCallback);
        } else {
          pressStartMs = now;
          longFired    = false;
          if(!hasDouble && !hasLong) {
            _applyPressState(true, ch, modulator, displayCallback);
            pressPhase = PressPhase::PRESS_ACTIVE;
          } else {
            pressPhase = PressPhase::WAIT;
          }
        }
      } else {
        // Release edge
        switch(pressPhase) {
          case PressPhase::WAIT:
            if(hasDouble) {
              // Released before long or double — start double-tap window
              releaseMs  = now;
              pressPhase = PressPhase::PENDING_DBL;
            } else {
              // HOLD configured but released before timeout — fire PRESS now
              _applyPressState(true, ch, modulator, displayCallback);
              _applyPressState(false, ch, modulator, displayCallback);
              if(hasRelease) _fireReleaseAction(ch, modulator, displayCallback);
              pressPhase = PressPhase::IDLE;
            }
            break;
          case PressPhase::PRESS_ACTIVE:
            _applyPressState(false, ch, modulator, displayCallback);
            if(hasRelease) _fireReleaseAction(ch, modulator, displayCallback);
            pressPhase = PressPhase::IDLE;
            break;
          case PressPhase::LONG_ACTIVE:
            _applyExtraAction(0, false, ch, modulator, displayCallback);
            longFired  = false;
            pressPhase = PressPhase::IDLE;
            break;
          case PressPhase::DBL_ACTIVE:
            _applyExtraAction(1, false, ch, modulator, displayCallback);
            pressPhase = PressPhase::IDLE;
            break;
          default:
            pressPhase = PressPhase::IDLE;
            break;
        }
      }
    }

    // Long press detection (every tick while held)
    if(hasLong && !longFired && isPressed &&
       (pressPhase == PressPhase::WAIT || pressPhase == PressPhase::PRESS_ACTIVE)) {
      if((now - pressStartMs) >= FS_LONG_MS) {
        if(pressPhase == PressPhase::PRESS_ACTIVE)
          _applyPressState(false, ch, modulator, displayCallback);
        _applyExtraAction(0, true, ch, modulator, displayCallback);
        longFired  = true;
        pressPhase = PressPhase::LONG_ACTIVE;
      }
    }

    // Double-tap window timeout → treat as single PRESS
    if(pressPhase == PressPhase::PENDING_DBL && (now - releaseMs) >= FS_DBL_WIN_MS) {
      _applyPressState(true, ch, modulator, displayCallback);
      _applyPressState(false, ch, modulator, displayCallback);
      if(hasRelease) _fireReleaseAction(ch, modulator, displayCallback);
      pressPhase = PressPhase::IDLE;
    }
  }

  void simulatePress(bool pressed, uint8_t midiChannel, MidiCCModulator &modulator) {
    isPressed = pressed;
    _applyPressState(pressed, effectiveChannel(midiChannel), modulator, nullptr);
  }

  const char *toggleFootswitchMode(MidiCCModulator &modulator) {
    modeIndex = (modeIndex + 1) % NUM_MODES;
    const ModeInfo &m = modes[modeIndex];
    // fsChannel transition: entering keyboard resets to no-modifiers;
    // leaving keyboard restores global-channel sentinel
    if(!isKeyboard && m.isKeyboard) fsChannel = 0x00;
    else if(isKeyboard && !m.isKeyboard) fsChannel = 0xFF;
    mode        = m.mode;
    isModSwitch = m.isModSwitch;
    isLatching  = m.isLatching;
    isPC        = m.isPC;
    isNote      = m.isNote;
    isScene     = m.isScene;
    isSceneScroll = m.isSceneScroll;
    isSystem    = m.isSystem;
    isKeyboard  = m.isKeyboard;
    scrollPos      = 0;
    scrollLastSent = 0;
    modMode     = m;
    if(m.isInverted) modulator.setRestingHigh();
    else             modulator.setRestingLow();
    modulator.rampShape = m.shape;
    modulator.reset();
    isActivated = false;
    _setLED(false);
    return m.name;
  }
};

// ── Mode property helpers ─────────────────────────────────────────────────────

// Returns true if this mode needs a value-entry submenu (level 4) in mode select.
inline bool modeNeedsValueEntry(uint8_t modeIdx) {
  if(modeIdx >= NUM_MODES) return false;
  FootswitchMode m = modes[modeIdx].mode;
  return m != FootswitchMode::NoAction
      && m != FootswitchMode::TapTempo
      && m != FootswitchMode::PresetUp
      && m != FootswitchMode::PresetDown
      && m != FootswitchMode::Multi;
}

// Returns true if this mode also needs a velocity entry (level 5) — Note mode only.
inline bool modeNeedsVelocity(uint8_t modeIdx) {
  if(modeIdx >= NUM_MODES) return false;
  return modes[modeIdx].isNote;
}

// Returns true if this mode shows the Up/Down speed submenus (levels 6/7).
inline bool modeNeedsRampEntry(uint8_t modeIdx) {
  if(modeIdx >= NUM_MODES) return false;
  return modes[modeIdx].isModSwitch;
}

// ── Free mode-application helpers ────────────────────────────────────────────

inline void applyModeFlags(FSButton &btn, uint8_t idx) {
  if(idx >= NUM_MODES) return;
  const ModeInfo &m = modes[idx];
  // fsChannel is NOT touched here — caller (applyPreset) provides it from NVS.
  // Only user-initiated paths (applyModeIndex, toggleFootswitchMode) reset it.
  btn.modeIndex      = idx;
  btn.mode           = m.mode;
  btn.isModSwitch    = m.isModSwitch;
  btn.isLatching     = m.isLatching;
  btn.isPC           = m.isPC;
  btn.isNote         = m.isNote;
  btn.isScene        = m.isScene;
  btn.isSceneScroll  = m.isSceneScroll;
  btn.isSystem       = m.isSystem;
  btn.isKeyboard     = m.isKeyboard;
  btn.scrollPos      = 0;
  btn.scrollLastSent = 0;
  btn.modMode        = m;
  btn.isActivated    = false;
  btn.ledState       = false;
  if(!m.isModSwitch && !m.isKeyboard && btn.midiNumber == PB_SENTINEL)
    btn.midiNumber = 0;
}

inline void applyModeIndex(FSButton &btn, uint8_t idx, MidiCCModulator *mod = nullptr) {
  // User-initiated mode change: transition fsChannel (keyboard ↔ MIDI modes)
  const ModeInfo &nm = modes[idx];
  if(!btn.isKeyboard && nm.isKeyboard) btn.fsChannel = 0x00;
  else if(btn.isKeyboard && !nm.isKeyboard) btn.fsChannel = 0xFF;
  applyModeFlags(btn, idx);
  if(mod) {
    const ModeInfo &m = modes[idx];
    if(m.isInverted) mod->setRestingHigh();
    else             mod->setRestingLow();
    mod->rampShape = m.shape;
    mod->reset();
  }
}
