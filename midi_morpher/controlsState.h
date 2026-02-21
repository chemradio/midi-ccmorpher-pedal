#ifndef CONTROLS_STATE_H
#define CONTROLS_STATE_H

struct ControlsState
{
    bool fs1Pressed = false;
    bool fs2Pressed = false;
    bool hotswitchPressed = false;
    bool extfs1Pressed = false;
    bool extfs2Pressed = false;
    bool encoderPressed = false;
    //
    bool fs1Activated = false;
    bool fs2Activated = false;
    bool hotswitchActivated = false;
    bool extfs1Activated = false;
    bool extfs2Activated = false;
    bool encoderActivated = false;
};

#endif