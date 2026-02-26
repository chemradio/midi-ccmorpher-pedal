#pragma once
#include "config.h"

enum class Direction
{
    UP,
    DOWN
};

struct MidiCCRamp
{
    //
    bool hotSwitchLatching = false;
    //
    bool isRamping = false;
    unsigned long rampUpMs = 1;
    unsigned long rampDownMs = 5000;
    //
    uint8_t rampProgress = 0;     // 0-255
    uint8_t rampProgressMIDI = 0; // 0-127
    //
    unsigned long lastMidiSendTime = 0;
    const unsigned long midiSendInterval = 5;

    Direction currentDirection = Direction::UP;
    Direction previousDirection = Direction::DOWN;

    void flipDirections()
    {
        if (currentDirection == Direction::UP)
            currentDirection == Direction::DOWN;
        else
            currentDirection == Direction::UP;

        if (previousDirection == Direction::UP)
            previousDirection == Direction::DOWN;
        else
            previousDirection == Direction::UP;
    }
    void startRamp()
    {
        uint8_t targetValue = 127;
        if (rampProgress == 127)
        {
            targetValue = 0
        }
    }

    void updateRamp()
    {
    }
}