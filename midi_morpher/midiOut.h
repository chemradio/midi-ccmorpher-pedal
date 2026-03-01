#pragma once
#include <Arduino.h>
// #include <Adafruit_TinyUSB.h>
// Adafruit_USBD_MIDI usbMIDI;

// ----- Configure transports -----
// #define USE_DIN_MIDI
// #define USE_USB_MIDI

// ----- Core send function -----
inline void sendMIDI(uint8_t channel, bool isPC, uint8_t number, uint8_t value = 0)
{

    // Serial.println(millis());
    // if (!isPC)
    // {
    //     Serial.print(" Value: ");
    //     Serial.println(value);
    // }

    // #ifdef USE_DIN_MIDI
    if (isPC)
    {
        Serial1.write(0xC0 | channel);
        Serial1.write(number);
    }
    else
    {
        Serial1.write(0xB0 | channel);
        Serial1.write(number);
        Serial1.write(value);
    }
    // #endif

    // #ifdef USE_USB_MIDI
    //     if (isPC)
    //     {
    //         usbMIDI.sendProgramChange(number, channel + 1);
    //     }
    //     else
    //     {
    //         usbMIDI.sendControlChange(number, value, channel + 1);
    //     }
    // #endif
}
