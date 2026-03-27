#pragma once
#include "config.h"
#include "midiOut.h"
#include "sharedTypes.h"
#include <Adafruit_NeoPixel.h>

enum RampShape {
  SHAPE_LINEAR,
  SHAPE_EXP,
  SHAPE_SINE,
  SHAPE_SQUARE
};

struct MidiCCModulator {
  bool switchPressed = false;
  bool inverted = false;
  bool latching = false;
  bool isLatched = false;
  bool isActivated = false;
  uint8_t targetValue = 0;
  uint8_t currentValue = 0;
  unsigned long rampUpTimeMs = DEFAULT_RAMP_SPEED;
  unsigned long rampDownTimeMs = DEFAULT_RAMP_SPEED;
  unsigned long rampStartTime = 0;
  uint8_t rampStartValue = 0;
  unsigned long currentRampDuration = 0;
  bool isModulating = false;
  uint8_t midiChannel = 0;
  uint8_t midiCCNumber = 25;
  RampShape rampShape = RampShape::SHAPE_LINEAR;
  ModulationType modType = ModulationType::RAMPER;

  float lfoRateHz = 1.0f;               // cycles per second
  unsigned long lfoStartTime = 0;       // set to millis() when press() is called
  bool lfoFinishing = false;            // true when completing final cycle before stopping
  unsigned long randomIntervalMs = 300; // ms between random jumps
  unsigned long lastRandomTime = 0;
  uint8_t randomMin = 0;   // lower bound for random target (0-127)
  uint8_t randomMax = 127; // upper bound for random target (0-127)
  uint8_t stepperSteps = 10;

  void updateRamper();
  void updateLFO();
  void updateStepper();
  void updateRandomStepper();

  void invert() {
    inverted = !inverted;
    currentValue = inverted ? 127 : 0;
    targetValue = inverted ? 0 : 127;
    rampStartValue = currentValue;
    rampStartTime = millis();
  }

  // t is 0.0–1.0, returns shaped 0.0–1.0
  float shapeRamp(float t, RampShape shape) {
    switch(shape) {
    case SHAPE_EXP:
      return t * t;

    case SHAPE_SINE:
      // S-curve: slow start, fast middle, slow end
      return 0.5f - 0.5f * cosf(t * M_PI);

    case SHAPE_SQUARE:
      // Snaps to target halfway through
      return t < 0.5f ? 0.0f : 1.0f;

    case SHAPE_LINEAR:
    default:
      return t;
    }
  }

  void reset() {
    currentValue = inverted ? 127 : 0;
    targetValue = inverted ? 0 : 127;
    switchPressed = false;
    isActivated = false;
    isModulating = false;
    lfoStartTime = 0;
    lastRandomTime = 0;
  }

  void _setLED(bool state) {
    digitalWrite(HS_LED, state);
  }

  void setLatch(bool enabled) {
    latching = enabled;
    if(currentValue > 63) {
      inverted = true;
    } else {
      inverted = false;
    }
    _setLED(false);
  }

  void setCurveType(bool isExp) {
  }

  void setCCNumber(uint8_t ccNumber) {
    midiCCNumber = ccNumber;
  }

  void setMidiChannel(uint8_t channel) {
    midiChannel = channel; // Fixed: was assigning to itself
  }

  void setRampTimeUp(unsigned long upTime) {
    rampUpTimeMs = upTime;
  }

  void setRampTimeDown(unsigned long downTime) {
    rampDownTimeMs = downTime;
  }

  void calcAndStartRamp() {
    if(isActivated) {
      targetValue = inverted ? 0 : 127;
    } else {
      targetValue = inverted ? 127 : 0;
    }

    rampStartTime = millis();
    rampStartValue = currentValue;
    isModulating = true;
  }

  void press() {
    if(switchPressed)
      return;
    switchPressed = true;

    if(latching) {
      isLatched = !isLatched;
      isActivated = !isActivated;
    } else {
      isActivated = true;
    }

    lfoStartTime = millis();
    lfoFinishing = false;
    _setLED(isActivated);
    calcAndStartRamp();
  }

  void release() {
    if(!switchPressed)
      return;
    switchPressed = false;

    if(latching)
      return;

    isActivated = false;
    _setLED(isActivated);

    if(modType == ModulationType::LFO) {
      lfoFinishing = true;
      isModulating = true;
    }
    calcAndStartRamp();
  }

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
    return;
  }
};

#include "modulators/lfo.h"
#include "modulators/ramper.h"
#include "modulators/randomStepper.h"
#include "modulators/stepper.h"