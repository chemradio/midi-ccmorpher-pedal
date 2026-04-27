#pragma once
#include "../config.h"
#include <Adafruit_NeoPixel.h>

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