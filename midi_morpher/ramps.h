#pragma once
#include "config.h"

struct MidiCCRamp
{
    bool isRamping = false;
    unsigned long rampUpMs = 1;
    unsigned long rampDownMs = 5000;
    //
    uint8_t rampProgress = 0;     // 0-255
    uint8_t rampProgressMIDI = 0; // 0-127
}