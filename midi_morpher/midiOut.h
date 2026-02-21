#ifndef MIDIOUT_H
#define MIDIOUT_H
#include <Arduino.h>
// #include <Adafruit_TinyUSB.h>
// Adafruit_USBD_MIDI usbMIDI;

// ----- Configure transports -----
#define USE_DIN_MIDI
// #define USE_USB_MIDI

// ----- Core send function -----
inline void sendMIDI(uint8_t channel, bool isPC, uint8_t number, uint8_t value)
{
    channel = constrain(channel, 0, 15);

#ifdef USE_DIN_MIDI
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
#endif

#ifdef USE_USB_MIDI
    if (isPC)
    {
        usbMIDI.sendProgramChange(number, channel + 1);
    }
    else
    {
        usbMIDI.sendControlChange(number, value, channel + 1);
    }
#endif

    // #ifdef USE_USB_MIDI
    //     if (isPC)
    //     {
    //         // Program Change (CIN 0x0C)
    //         uint8_t buf[4];
    //         buf[0] = 0x0C;               // CIN: Program Change
    //         buf[1] = 0xC0 | channel;     // Status byte
    //         buf[2] = number;             // Program number
    //         buf[3] = 0x00;               // Unused
    //         usbMIDI.send(buf, 4);
    //     }
    //     else
    //     {
    //         // Control Change (CIN 0x0B)
    //         uint8_t buf[4];
    //         buf[0] = 0x0B;               // CIN: Control Change
    //         buf[1] = 0xB0 | channel;     // Status byte
    //         buf[2] = number;             // CC number
    //         buf[3] = value;              // CC value
    //         usbMIDI.send(buf, 4);
    //     }
    // #endif
}

#endif