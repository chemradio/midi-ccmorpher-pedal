#ifndef PEDAL_STATE_H
#define PEDAL_STATE_H

struct PedalState
{
    // probably don't need these
    bool fs1Pressed = false;
    bool fs2Pressed = false;
    bool hotswitchPressed = false;
    bool extfs1Pressed = false;
    bool extfs2Pressed = false;
    bool encoderPressed = false;

    // for latching states: midi CC or HotSwitch. redundants included
    bool fs1Activated = false;
    bool fs2Activated = false;
    bool hotswitchActivated = false;
    bool extfs1Activated = false;
    bool extfs2Activated = false;
    bool encoderActivated = false;

    // pedal globals
    uint8_t midiChannel = 1;
    bool rampLinearCurve = true;
    bool rampDirectionUp = true;
    bool hotSwitchLatching = false;
    bool settingsLocked = false;
    float rampSpeed = 1.0;
    float rampMinSpeedSeconds = 0.1;
    float rampMaxSpeedSeconds = 5.0;

    // PC or CC + values
    bool fs1IsPC = true;
    bool fs2IsPC = true;
    bool hotswitchIsPC = false;
    bool extfs1IsPC = true;
    bool extfs2IsPC = true;

    // values
    uint8_t fs1Value = 0;
    uint8_t fs2Value = 1;
    uint8_t hotswitchValue = 6;
    uint8_t extfs1Value = 2;
    uint8_t extfs2Value = 3;

    // temporary states for ramp
    uint8_t rampProgress = 0;
    bool ramping = false;
    bool rampingUp = false;
    bool rampingDown = false;
};

#endif