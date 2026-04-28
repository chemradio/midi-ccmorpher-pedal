#pragma once
#include "../config.h"
#include "../pedalState.h"
#include <Adafruit_NeoPixel.h>

// ── Status pixel (onboard RGB, 1 pixel) ──────────────────────────────────────
inline Adafruit_NeoPixel pixel(NEOPIXEL_COUNT, RGB_PIN, NEO_GRB + NEO_KHZ800);

void initNeoPixel() {
  pixel.begin();
  pixel.setBrightness(1);
}

void updateNeoPixel(uint8_t midiValue, bool enabled) {
  if(!enabled) {
    pixel.clear();
    pixel.show();
    return;
  }
  uint16_t hue = map(midiValue, 0, 127, 43690, 65535);
  uint32_t color = pixel.gamma32(pixel.ColorHSV(hue));
  pixel.setPixelColor(0, color);
  pixel.setBrightness(map(midiValue, 0, 127, 1, 20));
  pixel.show();
}

// ── FS + Tempo NeoPixel chain (7 pixels) ─────────────────────────────────────
inline Adafruit_NeoPixel fsLeds(FS_LEDS_COUNT, FS_LEDS_PIN, NEO_GRB + NEO_KHZ800);

void initFSLeds() {
  fsLeds.begin();
  fsLeds.clear();
  fsLeds.show();
}

// Returns the base color for a footswitch based on its mode category.
static uint32_t _fsBaseColor(const FSButton &btn) {
  if(btn.mode == FootswitchMode::TapTempo)
    return fsLeds.Color(200, 0, 150);        // magenta

  switch(btn.modMode.modType) {
    case RAMPER:  return fsLeds.Color(255, 80, 0);   // orange
    case LFO:     return fsLeds.Color(0, 80, 255);   // blue
    case STEPPER: return fsLeds.Color(0, 200, 200);  // cyan
    case RANDOM:  return fsLeds.Color(0, 200, 60);   // green
    default: break;
  }

  if(btn.modMode.isScene)   return fsLeds.Color(200, 170, 0);  // yellow
  if(btn.modMode.isSystem)  return fsLeds.Color(120, 0, 220);  // purple
  if(btn.modMode.isKeyboard)return fsLeds.Color(120, 0, 220);  // purple

  using F = FootswitchMode;
  if(btn.mode == F::System || btn.mode == F::Multi ||
     btn.mode == F::NoAction || btn.mode == F::PresetUp ||
     btn.mode == F::PresetDown || btn.mode == F::PresetNum)
    return fsLeds.Color(120, 0, 220);                           // purple

  return fsLeds.Color(220, 220, 220);                           // white (basic)
}

// Scales an RGB color by a 0–255 brightness factor.
static uint32_t _dimColor(uint32_t c, uint8_t bright) {
  uint8_t r = (uint8_t)(((c >> 16) & 0xFF) * bright / 255);
  uint8_t g = (uint8_t)(((c >>  8) & 0xFF) * bright / 255);
  uint8_t b = (uint8_t)(( c        & 0xFF) * bright / 255);
  return fsLeds.Color(r, g, b);
}

void updateFSLeds(const PedalState &pedal, bool tempoOn) {
  if(pedal.globalSettings.ledMode == LED_MODE_OFF) {
    fsLeds.clear();
    fsLeds.show();
    return;
  }

  for(int i = 0; i < 6; i++) {
    const FSButton &btn = pedal.buttons[i];
    const MidiCCModulator &mod = pedal.modForFS(i);
    bool isMod = (btn.modMode.modType != NOMODULATION);

    // Show animation if the footswitch is logically on OR its modulator is
    // still running (ramp-down after release). For shared modulators, only
    // the last-pressed footswitch owns the animation.
    bool modRunning = isMod && mod.isModulating &&
                      (pedal.globalSettings.perFsModulator ||
                       pedal.lastActiveFSIndex == i);
    bool lit = btn.ledState || modRunning;

    if(!lit) {
      fsLeds.setPixelColor(i, 0);
      continue;
    }

    uint32_t base = _fsBaseColor(btn);

    if(isMod) {
      // Animate brightness from modulator value: dark floor so the LED is
      // always visibly on, peaks at full base color.
      uint8_t bright = (uint8_t)map((long)mod.currentValue, 0, MOD_MAX_14BIT, 12, 255);
      fsLeds.setPixelColor(i, _dimColor(base, bright));
    } else {
      fsLeds.setPixelColor(i, base);
    }
  }

  // Pixel 6: tempo flash (red), gated by the global tempo-LED enable flag.
  fsLeds.setPixelColor(6,
    (pedal.globalSettings.tempoLedEnabled && tempoOn)
      ? fsLeds.Color(255, 0, 0) : 0);

  fsLeds.show();
}
