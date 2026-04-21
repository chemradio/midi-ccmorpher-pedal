#pragma once
#include <Adafruit_NeoPixel.h>
#include "../controls/pots.h"

inline Adafruit_NeoPixel pixel(NEOPIXEL_COUNT, RGB_PIN, NEO_GRB + NEO_KHZ800);

void initNeoPixel()
{
    pixel.begin();
    pixel.setBrightness(1); // Adjust brightness (0-255)
}

void updateNeoPixel(uint8_t midiValue, AnalogPot *pots, bool enabled)
{
    if(!enabled) { pixel.clear(); pixel.show(); return; }
    uint8_t target = midiValue;

    for (int i = 0; i < 2; i++)
    {
        if (pots[i].ccDisplayDirty)
        {
            target = pots[i].lastMidiValue;
            if ((millis() - pots[i].ccLastDisplayDirty) > 1000)
            {
                pots[i].ccDisplayDirty = false;
            }
            break;
        }
    }

    uint16_t hue = map(target, 0, 127, 43690, 0);
    // Convert HSV to RGB and set LED color
    uint32_t color = pixel.gamma32(pixel.ColorHSV(hue));
    pixel.setPixelColor(0, color);
    pixel.setBrightness(map(target, 0, 127, 1, 20));
    pixel.show();
}