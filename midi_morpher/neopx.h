#pragma once
#include <Adafruit_NeoPixel.h>

#define RGB_PIN 48   // Onboard RGB LED pin
#define NUM_PIXELS 1 // Only one LED

Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

void initNeoPixel()
{
    pixel.begin();
    pixel.setBrightness(10); // Adjust brightness (0-255)
}

void updateNeoPixel(uint8_t midiValue)
{
    uint16_t hue = map(midiValue, 0, 127, 43690, 0);
    // Convert HSV to RGB and set LED color
    uint32_t color = pixel.gamma32(pixel.ColorHSV(hue));
    pixel.setPixelColor(0, color);
    pixel.show();
}