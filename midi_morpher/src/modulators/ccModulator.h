#pragma once
#include "../config.h"
#include "../midi/midiOut.h"
#include "../sharedTypes.h"

enum RampShape {
  SHAPE_LINEAR,
  SHAPE_EXP,
  SHAPE_SINE,
  SHAPE_SQUARE
};

// Destination for the modulator output. CC emits 7-bit CC messages on
// midiCCNumber; PITCHBEND emits 14-bit Pitch Bend messages (rest = 8192 center,
// active = 0 or 16383 depending on inverted flag).
enum ModDest : uint8_t {
  DEST_CC,
  DEST_PITCHBEND
};

// Full 14-bit range used internally. CC modes downcast to 7-bit on emit.
static constexpr uint16_t MOD_MAX_14BIT    = 16383;
static constexpr uint16_t MOD_CENTER_14BIT = 8192;
// DIN-MIDI rate cap for Pitch Bend emits. 2ms → ~500 msg/s; DIN fits ~1040 msg/s.
static constexpr uint16_t MOD_PB_MIN_EMIT_MS = 2;

struct MidiCCModulator {
  bool switchPressed = false;
  bool restingHigh = false; // true = resting at top (inverted modes)
  bool latching = false;
  bool isActivated = false;
  bool isModulating = false;
  bool lfoFinishing = false;  // true when LFO is returning to rest after release/latch-off
  bool lfoTowardRest = false; // LFO direction: true = currently sweeping toward rest value

  uint16_t currentValue = 0;
  uint16_t targetValue = 0;
  uint16_t rampStartValue = 0;
  // User-configurable output range (CC dest only; PB dest stays 0/center/MAX).
  // Driven from per-FS ccLow/ccHigh (0..127) << 7. Defaults give the full
  // 0..MOD_MAX_14BIT range, matching previous behavior.
  uint16_t minValue14 = 0;
  uint16_t maxValue14 = MOD_MAX_14BIT;
  uint8_t  stepperSteps = 10;
  uint8_t  midiChannel = 0;
  uint8_t  midiCCNumber = 25;

  // Output destination & emit-change cache. Reset on state change.
  ModDest  destType      = DEST_CC;
  uint8_t  lastEmittedCC = 0xFF;     // 0xFF = never emitted (forces first emit)
  uint16_t lastEmittedPB = 0xFFFF;   // 0xFFFF = never emitted
  unsigned long lastEmitMs = 0;

  unsigned long rampUpTimeMs = DEFAULT_RAMP_SPEED;
  unsigned long rampDownTimeMs = DEFAULT_RAMP_SPEED;
  unsigned long rampStartTime = 0;
  unsigned long lastRandomTime = 0;

  RampShape rampShape = SHAPE_LINEAR;
  ModulationType modType = ModulationType::RAMPER;

  // ── Forward declarations for modulator update functions ──────────────────
  void updateRamper();
  void updateLFO();
  void updateStepper();
  void updateRandomStepper();

  // ── Rest position ─────────────────────────────────────────────────────────
  // For CC: rest = 0 or 16383 depending on inverted. For PB: rest is always
  // 8192 (center), active end swaps based on inverted.

  uint16_t restVal()   const {
    if(destType == DEST_PITCHBEND) return MOD_CENTER_14BIT;
    return restingHigh ? maxValue14 : minValue14;
  }
  uint16_t activeVal() const {
    if(destType == DEST_PITCHBEND) return restingHigh ? 0 : MOD_MAX_14BIT;
    return restingHigh ? minValue14 : maxValue14;
  }

  void setRestingLow() { // normal: rests at 0 (or center for PB)
    restingHigh = false;
    currentValue = restVal();
    targetValue  = activeVal();
  }

  void setRestingHigh() { // inverted
    restingHigh = true;
    currentValue = restVal();
    targetValue  = activeVal();
  }

  // ── Ramp shape ────────────────────────────────────────────────────────────
  // t is 0.0–1.0, returns shaped 0.0–1.0.

  float shapeRamp(float t, RampShape shape) {
    switch(shape) {
    case SHAPE_EXP:
      return t * t;
    case SHAPE_SINE:
      return 0.5f - 0.5f * cosf(t * M_PI);
    case SHAPE_SQUARE:
      return t < 0.5f ? 0.0f : 1.0f;
    case SHAPE_LINEAR:
    default:
      return t;
    }
  }

  // ── State control ─────────────────────────────────────────────────────────

  void reset() {
    currentValue  = restVal();
    targetValue   = activeVal();
    switchPressed = false;
    isActivated   = false;
    isModulating  = false;
    lfoFinishing  = false;
    lfoTowardRest = false;
    lastRandomTime = 0;
    // Invalidate emit cache so the next real change always emits.
    lastEmittedCC = 0xFF;
    lastEmittedPB = 0xFFFF;
  }

  void setLatch(bool enabled) {
    latching = enabled;
  }

  void setCCNumber(uint8_t ccNumber) {
    midiCCNumber = ccNumber;
  }

  void setMidiChannel(uint8_t channel) {
    midiChannel = channel;
  }

  void setRampTimeUp(unsigned long ms) { rampUpTimeMs = ms; }
  void setRampTimeDown(unsigned long ms) { rampDownTimeMs = ms; }

  // ── Ramp helpers ──────────────────────────────────────────────────────────

  // Proportional interpolation step shared by Ramper, Stepper, and LFO.
  // Precondition: currentValue != targetValue.
  // Returns true  when the ramp is complete (outValue = targetValue).
  // Returns false when in progress         (outValue = shaped interpolation).
  bool calcRampValue(uint16_t &outValue) {
    bool     goingUp  = (targetValue > rampStartValue);
    uint16_t distance = goingUp ? (uint16_t)(targetValue - rampStartValue)
                                : (uint16_t)(rampStartValue - targetValue);
    unsigned long fullDur = goingUp ? rampUpTimeMs : rampDownTimeMs;
    unsigned long dur     = (unsigned long)(fullDur * ((float)distance / (float)MOD_MAX_14BIT));

    if (dur <= 1) { outValue = targetValue; return true; }

    unsigned long elapsed = millis() - rampStartTime;
    if (elapsed >= dur)  { outValue = targetValue; return true; }

    float   t      = (float)elapsed / (float)dur;
    float   shaped = shapeRamp(t, rampShape);
    int32_t delta  = (int32_t)targetValue - (int32_t)rampStartValue;
    int32_t v      = (int32_t)rampStartValue + (int32_t)((float)delta * shaped);
    if(v < 0) v = 0;
    if(v > (int32_t)MOD_MAX_14BIT) v = MOD_MAX_14BIT;
    outValue = (uint16_t)v;
    return false;
  }

  // ── Output emit ───────────────────────────────────────────────────────────
  // Modulators call emit() whenever currentValue is updated. CC-dest downcasts
  // 14-bit → 7-bit and only sends on 7-bit change (preserves pre-existing CC
  // msg rate). PB-dest sends on 14-bit change, gated by MOD_PB_MIN_EMIT_MS so
  // a fast LFO cannot flood DIN at 31.25 kbaud. `force` bypasses the rate gate
  // for terminal emits (ramp finished) so the final value is never lost.
  void emit(bool force = false) {
    if(destType == DEST_PITCHBEND) {
      if(currentValue == lastEmittedPB) return;
      unsigned long now = millis();
      if(!force && (now - lastEmitMs) < MOD_PB_MIN_EMIT_MS) return;
      lastEmitMs    = now;
      lastEmittedPB = currentValue;
      sendPitchBend(midiChannel, currentValue);
    } else {
      uint8_t cc7 = (uint8_t)(currentValue >> 7);  // 0..127
      if(cc7 == lastEmittedCC) return;
      lastEmittedCC = cc7;
      sendMIDI(midiChannel, false, midiCCNumber, cc7);
    }
  }

  void calcAndStartRamp() {
    targetValue = isActivated ? activeVal() : restVal();
    rampStartTime = millis();
    rampStartValue = currentValue;
    isModulating = true;
  }

  // ── Press / Release ───────────────────────────────────────────────────────

  void press() {
    if(switchPressed)
      return;
    switchPressed = true;

    if(latching) {
      isActivated = !isActivated;
    } else {
      isActivated = true;
    }

    if(modType == ModulationType::LFO) {
      if(isActivated) {
        // Start LFO sweep from rest toward active peak.
        lfoTowardRest = false;
        lfoFinishing = false;
        targetValue = activeVal();
      } else {
        // Latching off: return smoothly to rest.
        lfoFinishing = true;
        lfoTowardRest = true;
        targetValue = restVal();
      }
      rampStartValue = currentValue;
      rampStartTime = millis();
      isModulating = true;
    } else {
      lfoFinishing = false;
      calcAndStartRamp();
    }
  }

  void release() {
    if(!switchPressed)
      return;
    switchPressed = false;
    if(latching)
      return;

    isActivated = false;

    if(modType == ModulationType::LFO) {
      // Momentary release: stop oscillating, ramp back to rest.
      lfoFinishing = true;
      lfoTowardRest = true;
      targetValue = restVal();
      rampStartValue = currentValue;
      rampStartTime = millis();
      isModulating = true;
    } else {
      calcAndStartRamp();
    }
  }

  // ── Main update ───────────────────────────────────────────────────────────

  void update() {
    if(!isModulating)
      return;
    switch(modType) {
    case ModulationType::RAMPER:
      updateRamper();
      return;
    case ModulationType::LFO:
      updateLFO();
      return;
    case ModulationType::STEPPER:
      updateStepper();
      return;
    case ModulationType::RANDOM:
      updateRandomStepper();
      return;
    }
  }
};

#include "lfo.h"
#include "ramper.h"
#include "randomStepper.h"
#include "stepper.h"
