#pragma once
#include "config.h"
#include "footswitchObject.h"
#include "pedalState.h"
#include "ramps.h"

FSButton hotswitch = FSButton(HS_PIN, HS_LED, "Hotswitch", 0);

bool lastHotSwitchState = false;

void handleHotswitch(MidiCCRamp &ramp)
{
    if (hotswitch.readState() && !lastHotSwitchState)
    {
        lastHotSwitchState = true;
        ramp.press();
        return;
    }
    if (!hotswitch.readState() && lastHotSwitchState)
    {
        lastHotSwitchState = false;
        ramp.release();
        return;
    }
}