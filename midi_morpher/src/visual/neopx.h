#pragma once
#include "../config.h"
#include "../pedalState.h"
#include <Adafruit_NeoPixel.h>

// ── FS + Tempo NeoPixel chain (7 pixels) ─────────────────────────────────────
inline Adafruit_NeoPixel fsLeds(FS_LEDS_COUNT, FS_LEDS_PIN, NEO_GRB + NEO_KHZ800);

#define NEO_LOW_BRIGHTNESS 4 // dim idle level (~3%)

void initFSLeds() {
  fsLeds.begin();
  fsLeds.clear();
  fsLeds.show();
}

// Returns the base color for a footswitch based on its mode category.
static uint32_t _fsBaseColor(const FSButton &btn) {
  if(btn.mode == FootswitchMode::TapTempo)
    return fsLeds.Color(200, 0, 150); // magenta

  switch(btn.modMode.modType) {
  case RAMPER:
    return fsLeds.Color(255, 80, 0); // orange
  case LFO:
    return fsLeds.Color(0, 80, 255); // blue
  case STEPPER:
    return fsLeds.Color(0, 200, 200); // cyan
  case RANDOM:
    return fsLeds.Color(0, 200, 60); // green
  default:
    break;
  }

  if(btn.modMode.isScene)
    return fsLeds.Color(200, 170, 0); // yellow
  if(btn.modMode.isSystem)
    return fsLeds.Color(120, 0, 220); // purple
  if(btn.modMode.isKeyboard)
    return fsLeds.Color(120, 0, 220); // purple

  using F = FootswitchMode;
  if(btn.mode == F::System || btn.mode == F::Multi ||
     btn.mode == F::NoAction || btn.mode == F::PresetUp ||
     btn.mode == F::PresetDown || btn.mode == F::PresetNum)
    return fsLeds.Color(120, 0, 220); // purple

  return fsLeds.Color(220, 220, 220); // white (basic)
}

// Color that replaces the base color while a modulator is actively sweeping.
static uint32_t _fsModulatingColor(const FSButton &btn) {
  switch(btn.modMode.modType) {
  case RAMPER:
    return fsLeds.Color(255, 0, 0); // orange → red
  case LFO:
    return fsLeds.Color(255, 0, 50); // blue   → red
  case STEPPER:
    return fsLeds.Color(220, 180, 0); // cyan   → yellow
  case RANDOM:
    return fsLeds.Color(200, 200, 200); // green  → white
  default:
    return fsLeds.Color(255, 0, 0);
  }
}

// Scales an RGB color by a 0–255 brightness factor.
static uint32_t _dimColor(uint32_t c, uint8_t bright) {
  uint8_t r = (uint8_t)(((c >> 16) & 0xFF) * bright / 255);
  uint8_t g = (uint8_t)(((c >> 8) & 0xFF) * bright / 255);
  uint8_t b = (uint8_t)((c & 0xFF) * bright / 255);
  return fsLeds.Color(r, g, b);
}

// Linearly interpolate between two colors; t=0 → a, t=255 → b.
static uint32_t _blendColor(uint32_t a, uint32_t b, uint8_t t) {
  uint8_t ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
  uint8_t br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
  return fsLeds.Color(
      (uint8_t)(ar + (int)(br - ar) * t / 255),
      (uint8_t)(ag + (int)(bg - ag) * t / 255),
      (uint8_t)(ab + (int)(bb - ab) * t / 255));
}

// Per-FS visual blend: 0=base color, 255=modulating color.
// Moves toward the CC-derived target at a fixed rate so the transition is
// always smooth regardless of how fast (or discontinuously) the CC jumps.
static uint8_t  _ledVisBlend[6]  = {0};
static uint32_t _ledBlendLastMs  = 0;
#define LED_FADE_MS 100  // ms for full 0→255 or 255→0 visual transition

void updateFSLeds(const PedalState &pedal, bool tempoOn) {
  if(pedal.globalSettings.ledMode == LED_MODE_OFF) {
    fsLeds.clear();
    fsLeds.show();
    return;
  }

  bool conservative = (pedal.globalSettings.ledMode == LED_MODE_CONSERVATIVE);
  bool onMode = (pedal.globalSettings.ledMode == LED_MODE_ON);

  uint32_t now = millis();
  uint32_t dt  = now - _ledBlendLastMs;
  if(dt > 50) dt = 50;  // cap to handle first-call / long pauses
  _ledBlendLastMs = now;
  uint8_t maxStep = (uint8_t)((255UL * dt + LED_FADE_MS - 1) / LED_FADE_MS);
  if(maxStep < 1) maxStep = 1;

  for(int i = 0; i < 6; i++) {
    const FSButton &btn = pedal.buttons[i];
    const MidiCCModulator &mod = pedal.modForFS(i);
    bool isMod = (btn.modMode.modType != NOMODULATION);

    bool ownsMod = pedal.globalSettings.perFsModulator || pedal.lastActiveFSIndex == i;

    uint32_t base = _fsBaseColor(btn);

    if(isMod && (onMode || conservative)) {
      // Target blend: distance of currentValue from its resting position.
      uint16_t dist = mod.restingHigh
                          ? (MOD_MAX_14BIT - mod.currentValue)
                          : mod.currentValue;
      uint8_t targetBlend = (uint8_t)(dist * 255UL / MOD_MAX_14BIT);
      // Only animate for the FS that owns the modulator (shared-mod case).
      if(!ownsMod) targetBlend = 0;

      // Smooth _ledVisBlend[i] toward target.
      uint8_t &vb = _ledVisBlend[i];
      if(vb < targetBlend)      vb = (targetBlend - vb <= maxStep) ? targetBlend : vb + maxStep;
      else if(vb > targetBlend) vb = (vb - targetBlend <= maxStep) ? targetBlend : vb - maxStep;

      if(vb > 0) {
        uint32_t blended = _blendColor(base, _fsModulatingColor(btn), vb);
        // Active: full brightness. Fading back: dim that scales down with blend.
        fsLeds.setPixelColor(i, btn.ledState ? blended
            : _dimColor(blended, NEO_LOW_BRIGHTNESS + (uint8_t)(vb / 4)));
      } else if(btn.ledState) {
        fsLeds.setPixelColor(i, base);
      } else {
        fsLeds.setPixelColor(i, _dimColor(base, NEO_LOW_BRIGHTNESS));
      }
      continue;
    }

    if(onMode) {
      // Non-mod in ON mode: dim idle, full when active.
      fsLeds.setPixelColor(i, btn.ledState ? base : _dimColor(base, NEO_LOW_BRIGHTNESS));
      continue;
    }

    // Conservative non-mod: off when idle, full when active.
    bool modRunning = isMod && mod.isModulating && ownsMod;
    bool lit = btn.ledState || modRunning;
    if(!lit) {
      fsLeds.setPixelColor(i, 0);
      continue;
    }
    fsLeds.setPixelColor(i, base);
  }

  // Pixel 6: tempo flash (red), gated by the global tempo-LED enable flag.
  fsLeds.setPixelColor(6,
                       (pedal.globalSettings.tempoLedEnabled && tempoOn)
                           ? fsLeds.Color(255, 0, 0)
                           : 0);

  fsLeds.show();
}
