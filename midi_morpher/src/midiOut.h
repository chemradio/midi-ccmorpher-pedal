#pragma once
#include <USB.h>
#include <USBMIDI.h>

extern USBMIDI midi;

inline void sendMIDI(uint8_t channel, bool isPC, uint8_t number, uint8_t value = 0)
{
    if (isPC)
    {
        // Program Change - DIN
        Serial1.write(0xC0 | channel);
        Serial1.write(number);

        // Program Change - USB
        midi.programChange(number, channel + 1);
    }
    else
    {
        // Control Change - DIN
        Serial1.write(0xB0 | channel);
        Serial1.write(number);
        Serial1.write(value);

        // Control Change - USB
        midi.controlChange(number, value, channel + 1);
    }
}