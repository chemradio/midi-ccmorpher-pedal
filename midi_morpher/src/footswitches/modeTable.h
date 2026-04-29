#pragma once
#include "../config.h"
#include "../clock/midiClock.h"
#include "../controls/hidKeyboard.h"
#include "../modulators/ccModulator.h"
#include "../midi/midiOut.h"
#include "../sharedTypes.h"
#include "../scenes/multiScene.h"

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

inline constexpr uint8_t NUM_MODES = sizeof(modes) / sizeof(modes[0]);

// ── Mode categories (three-level encoder selection) ──────────────────────────
//
// autoSelect=true  → single mode, confirmed immediately on category press
// subGroupCount=0  → two nav levels: cat → variant
// subGroupCount>0  → three nav levels: cat → sub-group → variant
//                    modeIdx = firstIdx + sgIdx*subGroupSize + variantIdx

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
inline constexpr const char *sgSceneUnits[] = { "Helix", "QC", "Fractal", "Kemper" };
inline constexpr const char *varSnapScroll[]= { "Snapshot", "Scroll" };

inline constexpr ModeCategory modeCategories[] = {
  //  name,         auto,  first,count, sgCnt,sgSz, sgNames,      sgNotes,     varNames
  { "No Action",    true,  0,  1,  0, 0, nullptr,       nullptr,     nullptr      },
  { "PC",           true,  1,  1,  0, 0, nullptr,       nullptr,     nullptr      },
  { "CC",           false, 2,  3,  0, 0, nullptr,       nullptr,     varCC        },
  { "Note",         true,  5,  1,  0, 0, nullptr,       nullptr,     nullptr      },
  { "Ramp",         false, 6,  12, 6, 2, sgRampWave,    sgRampNotes, varMomLatch  },
  { "LFO",          false, 18, 6,  3, 2, sgLfoWave,     sgLfoNotes,  varMomLatch  },
  { "Stepper",      false, 24, 4,  2, 2, sgNormalInv,   sgNormNotes, varMomLatch  },
  { "Random",       false, 28, 4,  2, 2, sgNormalInv,   sgNormNotes, varMomLatch  },
  { "Scene/Snap",   false, 32, 8,  4, 2, sgSceneUnits,  nullptr,     varSnapScroll},
  { "Tap Tempo",    true,  40, 1,  0, 0, nullptr,       nullptr,     nullptr      },
  { "System",       true,  41, 1,  0, 0, nullptr,     nullptr,     nullptr      },
  { "Keyboard",     true,  42, 1,  0, 0, nullptr,     nullptr,     nullptr      },
  { "Multi",        false, 43, 1,  0, 0, nullptr,     nullptr,     nullptr      },
  { "Preset Nav",   false, 44, 3,  0, 0, nullptr,     nullptr,     varPresetNav },
};
inline constexpr uint8_t NUM_CATEGORIES = sizeof(modeCategories) / sizeof(modeCategories[0]);

// presetNavRequest / presetNavDirect now live in pedalState.h.
// Forward declarations so this header doesn't pull in pedalState.h.
extern int8_t presetNavRequest;
extern int8_t presetNavDirect;

inline uint8_t categoryForModeIndex(uint8_t modeIdx) {
  for(uint8_t i = 0; i < NUM_CATEGORIES; i++) {
    if(modeIdx >= modeCategories[i].firstIdx &&
       modeIdx <  modeCategories[i].firstIdx + modeCategories[i].count)
      return i;
  }
  return 0;
}

inline ModulationType getModulationType(FootswitchMode mode) {
  for(const auto &m : modes) {
    if(m.mode == mode) return m.modType;
  }
  return ModulationType::NOMODULATION;
}
