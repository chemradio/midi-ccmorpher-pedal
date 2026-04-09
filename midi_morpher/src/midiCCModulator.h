#pragma once
#include "config.h"
#include "midiOut.h"
#include "sharedTypes.h"

enum RampShape {
  SHAPE_LINEAR,
  SHAPE_EXP,
  SHAPE_SINE,
  SHAPE_SQUARE
};

struct MidiCCModulator {
  bool switchPressed = false;
  bool restingHigh = false; // true = resting at 127 (inverted modes)
  bool latching = false;
  bool isActivated = false;
  bool isModulating = false;
  bool lfoFinishing = false;  // true when LFO is returning to rest after release/latch-off
  bool lfoTowardRest = false; // LFO direction: true = currently sweeping toward rest value

  uint8_t currentValue = 0;
  uint8_t targetValue = 0;
  uint8_t rampStartValue = 0;
  uint8_t randomMin = 0;
  uint8_t randomMax = 127;
  uint8_t stepperSteps = 10;
  uint8_t midiChannel = 0;
  uint8_t midiCCNumber = 25;

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
  // Called from toggleFootswitchMode to configure inverted/normal mode.

  void setRestingLow() { // normal: rests at 0, sweeps to 127
    restingHigh = false;
    currentValue = 0;
    targetValue = 127;
  }

  void setRestingHigh() { // inverted: rests at 127, sweeps to 0
    restingHigh = true;
    currentValue = 127;
    targetValue = 0;
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
    currentValue = restingHigh ? 127 : 0;
    targetValue = restingHigh ? 0 : 127;
    switchPressed = false;
    isActivated = false;
    isModulating = false;
    lfoFinishing = false;
    lfoTowardRest = false;
    lastRandomTime = 0;
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

  uint8_t restVal()   const { return restingHigh ? 127 : 0;   }
  uint8_t activeVal() const { return restingHigh ? 0   : 127; }

  // Proportional interpolation step shared by Ramper, Stepper, and LFO.
  // Precondition: currentValue != targetValue.
  // Returns true  when the ramp is complete (outValue = targetValue).
  // Returns false when in progress         (outValue = shaped interpolation).
  bool calcRampValue(uint8_t &outValue) {
    bool     goingUp  = (targetValue > currentValue);
    uint8_t  distance = goingUp ? (targetValue - currentValue)
                                : (currentValue - targetValue);
    unsigned long fullDur = goingUp ? rampUpTimeMs : rampDownTimeMs;
    unsigned long dur     = (unsigned long)(fullDur * (distance / 127.0f));

    if (dur <= 1) { outValue = targetValue; return true; }

    unsigned long elapsed = millis() - rampStartTime;
    if (elapsed >= dur)  { outValue = targetValue; return true; }

    float   t      = (float)elapsed / (float)dur;
    float   shaped = shapeRamp(t, rampShape);
    int     delta  = (int)targetValue - (int)rampStartValue;
    outValue = (uint8_t)((int)rampStartValue + (int)((float)delta * shaped));
    return false;
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

#include "modulators/lfo.h"
#include "modulators/ramper.h"
#include "modulators/randomStepper.h"
#include "modulators/stepper.h"
